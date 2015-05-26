#include "Global.h"
#include "SyscallGlobal.h"
#include "StateDefs.h"
#include "CElfReader.h"
#include "ProcUtil.h"
#include "CStopState.h"
#include "CProcCtrl.h"
#include "CApp.h"
#include "CException.h"

#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string>
#include <memory>

#include <log4cpp/Category.hh>

#ifdef _DEBUG
#include <stdlib.h>
#include <sstream>
#include <fstream>
#endif

inline int CStopState::_Wait()
{
    return m_pProcCtrl->_Wait();
}

CStopState::CStopState(CProcCtrl* pProcCtrl) : CState(pProcCtrl), m_optSet(false)
{
}

CStopState::~CStopState()
{
}

// Case 1: SIGSTOP sent to us by ptrace() implicitly after fork()/vfork()/clone()
// Case 2: SIGSTOP coming from CProcCtrl::_PrepareForAttach()
// Case 3: SIGSTOP sent to us by parent to change our pgid. See CForkState.
// Case 4: SIGSTOP we sent to self on PTRACE_EVENT_EXEC. Happens every time execve() is called.
void CStopState::Execute(pid_t pid, int sigNum)
{
    using namespace log4cpp;

    siginfo_t siginfo;
    
    // PTRACE_GETSIGINFO succeeds only when the signal is explicitly delivered (kill(), raise(), sigqueue() etc.) but not if
    // it comes from ptrace() machinery. Wish there were a better way of distinguishing the two!
    if ( ptrace(PTRACE_GETSIGINFO, pid, 0, &siginfo) == -1 ) // Newly forked process
    {
        m_logger.noticeStream() << '[' << pid << "] CStopState::Execute() - Received SIGSTOP from ptrace()" << eol;
        m_logger.noticeStream().flush();
        _ConfigNewProcess(pid);
    }
    else
    {
        if ( siginfo.si_pid == getpid() && siginfo.si_code == SI_QUEUE )
        {
            _InsertSyscall(pid, siginfo.si_value.sival_int);
        }
        else // The source of SIGSTOP is unknown. TODO: Is it possible to prevent the signal from reaching the target? If yes, is it
             // the right thing to do? 
        {
            m_logger.warnStream() << '[' << pid << "] CStopState::Execute() - Ignoring SIGSTOP from an unknown source {" 
                << siginfo.si_pid << "}. si_code = " << siginfo.si_code << eol;
            m_logger.warnStream().flush();
        }
    }

    if ( ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CStopState::Execute() - PTRACE_SETOPTIONS - " << errMsg;
        throw ex;
    } 
}

void CStopState::Notify(int code, int data)
{
}

void CStopState::_ConfigNewProcess(pid_t pid) // That's how a child begins its life after being forked() off the parent
{
    using namespace log4cpp;

    // Both new processes and threads get a SIGSTOP from ptrace() but it seems only the one for processes matters. That is to say,
    // PTRACE_SETOPTIONS once set, applies to all the constituent threads as well even though each thread has its own pid (tid) and
    // gets a SIGSTOP after a clone(). Furthermore our caller Execute() filters away the signal meant for threads as not having
    // come from ptrace. TODO: Need to get a better understanding of SIGSTOP targetting threads.
    if ( !m_optSet )
    { 
        m_optSet = true;

        if ( ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE
                    | PTRACE_O_TRACEEXEC) == -1 )
        {
            CException ex(errno);
            ex << '[' << pid << "] CStopState::_ConfigNewProcess() - PTRACE_SETOPTIONS - " << errMsg;
            throw ex;
        }

        // Instruct the controlled process to lead a new group. Because the very first process spawned by virapix has already been made a group 
        // leader, we skip this step for that process. 
        if ( !m_pProcCtrl->_IsGrpLeader() )
        {
            _InsertSyscall(pid, SYS_setpgid);
        }
    }
}

void CStopState::_InsertSyscall(pid_t pid, int syscall)
{
    using namespace log4cpp;

    switch ( syscall )
    {
        case SYS_setpgid:
            m_logger.noticeStream() << '[' << pid << "] CStopState::_InsertSyscall() - SYS_setpgid" << eol;
            m_logger.noticeStream().flush();

            // We define a separate function instead of piggybacking on CProcCtrl::_SyscallInserted() because if something goes wrong before
            // we get to invoke _SyscallInserted() and pid dies, then the flag won't be set, CProcCtrl::_Start() won't wake up our creator thread
            // which will then block forever in CForkState::_OnFork(). 
            m_pProcCtrl->_PreInsertSyscall(SYS_setpgid);
            _PrepareChangePGID(pid);
            break;

        case SYS_mmap:
            m_logger.noticeStream() << '[' << pid << "] CStopState::_InsertSyscall() - SYS_mmap" << eol;
            m_logger.noticeStream().flush();
            _PreparePageReserve(pid);
            break;

        default: // Should never happen!
            m_logger.warnStream() << '[' << pid << "] CStopState::_InsertSyscall() - Unexpected qualifier for queued SIGSTOP!" << eol;
            m_logger.warnStream().flush();
            break;
    }

    return;
}

// Very much like _PreparePageReserve(), but much luckier. _PrepareChangePGID() is called immediately after fork(); in other words
// the entire memory map of the parent process (including loaded libraries like libc) is available. We fetch the address of setpgid()
// routine and bob's your uncle. The rationale behind changing the process group is discussed in CStopState::_Run().
void CStopState::_PrepareChangePGID(pid_t pid)
{
    using namespace log4cpp;
    using namespace std;

    SMemMap memMap;
    userreg_t setpgidAddress = 0;

    // Get the location in the process virtual address space where libc is mapped.
    try
    {
        string exeName;
        GetMemMap(pid, CApp::GetInstance()->GetLibcName(), memMap);
        userreg_t setpgidOffset = CApp::GetInstance()->GetSetpgidOffset(m_pProcCtrl->_Is64Bit());
        setpgidAddress = memMap.m_start + setpgidOffset;
    }

    catch ( const CException& ex )
    {
#ifdef _DEBUG
        stringstream command;
        command << "cat /proc/" << pid << "/maps > maps.libc.txt";
        system(command.str().c_str());
#endif
        CException ex2;
        ex2 << '[' << pid << "] CStopState::_PrepareChangePGID() - " << ex.StrError();
        throw ex2;
    }

    _SetRegs(pid, setpgidAddress, SYS_setpgid);
}

// The idea here is to reserve a page of memory in the child address space to store the modified file paths to be passed to the
// intercepted system calls. mmap() is the obvious choice, but the only shared library loaded at the completion of execve() is the 
// dynamic linker viz. ld-linux.so which would later load the coveted libc. The dynamic linker maintains its own copy of mmap() 
// (required to load other libraries). Sadly, mmap() is defined as "LOCAL" to the module and hence is not visible from outside 
// (dlsym() fails). Worse still, if ld-linux.so is stripped, .symtab section in the ELF binary is erased and so are the names of all 
// private (i.e. LOCAL) symbols. The .dynsym section survives, but doesn't hold any private symbol names. Hence:
// WARNING: This function will fail if ld-linux.so is stripped.
void CStopState::_PreparePageReserve(pid_t pid)
{
    using namespace std;

    SMemMap memMap;
    userreg_t mmapAddress = 0;

    // Get the location in the process virtual address space where ld is mapped.
    try
    {
        GetMemMap(pid, CApp::GetInstance()->GetLdsoName(), memMap);
        userreg_t mmapOffset = CApp::GetInstance()->GetMmapOffset(m_pProcCtrl->_Is64Bit());
        mmapAddress = memMap.m_start + mmapOffset;
    }

    catch ( const CException& ex )
    {
#ifdef _DEBUG
        stringstream command;
        command << "cat /proc/" << pid << "/maps > maps.ld.txt";
        system(command.str().c_str());
#endif
        CException ex2;
        ex2 << '[' << pid << "] CStopState::_PreparePageReserve() - " << ex.StrError();
        throw ex2;
    }

    _SetRegs(pid, mmapAddress, SYS_mmap);
}

void CStopState::_SetRegs(pid_t pid, userreg_t syscallAddr, int syscall)
{
    using namespace log4cpp;
    using namespace std;

    SUserRegs newRegs;
    auto_ptr<SUserRegs> pRegs(new SUserRegs);
    pRegs->m_is64Bit = m_pProcCtrl->_Is64Bit();

    string str;

    // Back up the current registers' state
    if ( ptrace(PTRACE_GETREGS, pid, 0, (user_regs_struct*)pRegs.get()) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CStopState::_SetRegs() - PTRACE_GETREGS - " << errMsg;
        throw ex;
    }

    // Set up the registers for the syscall
    newRegs = *pRegs;

    switch ( syscall )
    {
        case SYS_mmap:
            SetMmapRegs(pid, newRegs, syscallAddr);
            break;

        case SYS_setpgid:
            SetSetpgidRegs(pid, newRegs, syscallAddr);
            break;
    }

#ifdef _DEBUG
        string exeName;
        ofstream diskOut("mmap.addr.txt");
        diskOut << '[' << GetExePath(pid, exeName) << "] mmap() address: " << syscallAddr << endl << "Current instruction address: " 
           << GetIP(*pRegs.get()) << endl << flush;
        diskOut << '[' << exeName << "] New instruction address: " << GetIP(newRegs) << endl << flush;
#endif

    if ( ptrace(PTRACE_SETREGS, pid, 0, (user_regs_struct*) &newRegs) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CStopState::_SetRegs() - PTRACE_SETREGS - " << errMsg;
        throw ex;
    }

    m_pProcCtrl->m_pRegsBackup.reset();
    m_pProcCtrl->m_pRegsBackup = pRegs;
    m_pProcCtrl->_SyscallInserted(syscall);
}
