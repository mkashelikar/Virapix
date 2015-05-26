#include "StateDefs.h"
#include "SyscallGlobal.h"
#include "CSyscallState.h"
#include "CProcCtrl.h"
#include "CException.h"
#include "Functor.h"

#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <string>

#include <log4cpp/Category.hh>

inline bool CSyscallState::_IsInsertSyscallExit(bool entryExit)
{
    return ( entryExit && ((m_insertedSyscall == SYS_setpgid) || (m_insertedSyscall == SYS_mmap)) );
}

inline void CSyscallState::_NotifyParent(int sig)
{
    m_pProcCtrl->_NotifyParent(sig);
}

CSyscallState::CSyscallState(CProcCtrl* pProcCtrl) : CState(pProcCtrl), m_insertedSyscall(NOSYSCALL),
    m_intercept(CSig1(this, &CSyscallState::_SyscallUpdate)) 
{
}

CSyscallState::~CSyscallState()
{
}

// This method - admittedly an overkill for single threaded processes - is most useful for controlling multithreaded applications 
// where a syscall execution in one thread is likely to be interleaved with a syscall in another. In other words, we have multiple 
// consecutive entries/exits corresponding to each participating thread. Such a state cannot be tracked by a simple boolean. 
// m_sysCallers is a map of pid and the syscall the process is currently executing. The method adds/removes an item corresponding to 
// whether a syscall is entered or exited respectively by that thread. A thread being essentially a process in Linux, has a pid as 
// well. The function returns false if a syscall is entered and true otherwise. This function also deals with syscalls that never 
// return. 
inline bool CSyscallState::_ToggleSyscallState(pid_t pid, int sysCall)
{
    using namespace std;

    bool entryExit = false;
    map<pid_t, int>::iterator iter = m_sysCallers.find(pid);
    
    if ( iter != m_sysCallers.end() )
    {
        // The ***normal*** exit case; i.e. the syscall returned.
        if ( iter->second == sysCall )
        {
            entryExit = true;
            m_sysCallers.erase(iter);
        }
        else // This is a new syscall; the previous one didn't return (eg. sigreturn()). We update the entry with new syscall number.
        {
            m_sysCallers[pid] = sysCall;
        }
    }
    else
    {
        // The ***normal*** entry case; i.e. we are about to enter a syscall. The following expression adds the appropriate pair to
        // the map.
        m_sysCallers[pid] = sysCall;
    }
    
    return entryExit; 
}

void CSyscallState::_SyscallUpdate(int code, pid_t pid, int newSyscall)
{
    switch ( code )
    {
        // The syscall interceptor replaced a syscall with another. This is crucial since the entry/exit is determined by absence or 
        // presence of the syscall number in m_sysCallers.
        case NOTIFY_ALTSYSCALL:
            std::map<pid_t, int>::iterator iter = m_sysCallers.find(pid);
            assert(iter != m_sysCallers.end());
            iter->second = newSyscall;
            break;
    }
}

void CSyscallState::Execute(pid_t pid, int sigNum)
{
    SUserRegs userRegs;

    userRegs.m_is64Bit = m_pProcCtrl->_Is64Bit();

    if ( ptrace(PTRACE_GETREGS, pid, 0, (user_regs_struct*) &userRegs) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CSyscallState::Execute() - PTRACE_GETREGS - " << errMsg;
        throw ex;
    }

    // A syscall value of -1 implies an exception condition. TODO: Understand why ptrace sends SIGTRAP | 0x80 as if for a system call
    if ( GetSyscallNumber(userRegs) != -1 )
    {
        bool entryExit = _ToggleSyscallState(pid, GetSyscallNumber(userRegs));

        if ( _IsInsertSyscallExit(entryExit) )
        {
            // This is where we process results from the system calls we inserted earlier.
            _OnInsertSyscall(pid, userRegs);
        }
        else
        {
            // PrintRegs(pid, userRegs, m_pProcCtrl, entryExit);
            m_intercept.OnSyscall(pid, userRegs, entryExit);
        }
    }

    if ( ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CSyscallState::Execute() - PTRACE_SYSCALL - " << errMsg;
        throw ex;
    }
}

void CSyscallState::Notify(int code, int data)
{
    switch ( code )
    {
        case NOTIFY_SYSINSERT:
            m_insertedSyscall = data;
            break;
    }
}

void CSyscallState::_OnInsertSyscall(pid_t pid, user_regs_struct& userRegs)
{
    using namespace log4cpp;
    using namespace std;

    long status = GetRetStatus(userRegs);
    string str;

    if ( status < 0 )
    {
        CException ex(-status);
        ex << '[' << pid << "] CSyscallState::_OnInsertSyscall() - " << errMsg;  
        throw ex;
    }
    else
    {
        _OnSyscallReturn(pid, status);
    }
}

void CSyscallState::_OnSyscallReturn(pid_t pid, userreg_t status)
{
    using namespace std;

    switch ( m_insertedSyscall )
    {
        case SYS_setpgid:
            _OnSetpgidReturn(pid);
            m_pProcCtrl->_SyscallReturn(SYS_setpgid);
            break;

        case SYS_mmap:
            _OnMmapReturn(pid, status);
            break;
    }

    // Restore the register state saved prior to inserting the syscall.
    if ( ptrace(PTRACE_SETREGS, pid, 0, (user_regs_struct*) m_pProcCtrl->m_pRegsBackup.get()) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CSyscallState::_OnSyscallReturn() - PTRACE_SETREGS - " << errMsg;
        throw ex;
    }

    // Since _OnSetpgidReturn() sets up a call to mmap() in the controlled process, we require the original registers (the ones 
    // backed up when setpgid() was inserted by CFortState::_OnFork()) when mmap() returns. On the other hand, when mmap() returns,
    // we don't insert any more syscalls in the process address space and as such no longer require those registers.
    if ( m_insertedSyscall == SYS_mmap )
    {
        m_pProcCtrl->m_pRegsBackup.reset();
    }
}

void CSyscallState::_OnSetpgidReturn(pid_t pid)
{
    using namespace log4cpp;

    // Now that we lead our own process group, negate the value of m_pProcCtrl->m_pid so that we can receive events from our 
    // threads too. See the explanation in CProcCtrl::_Run() member function.
    m_pProcCtrl->m_pid = -m_pProcCtrl->m_pid;

    m_logger.noticeStream() << '[' << pid << "] CSyscallState::_OnSetpgidReturn() - New process group id " << getpgid(pid) << eol; 
    m_logger.noticeStream().flush();

    // Wake up our creator thread waiting in _OnFork().
    _NotifyParent(SIGWAKEPARENT1);

    // This may seem to be a waste since a call to execve() later will unmap the very region we are about to set up. Indeed. However,
    // if the newly forked process calls disk I/O syscalls ***before*** starting a new program, we should have a scratch pad ready to 
    // manipulate the function arguments. Secondly, if arguments to execve("new program") themselves need to be modified, it's 
    // essential to have the memory region ready to put the new values in. 
    sigval sinfo;
    sinfo.sival_int = SYS_mmap;

    if ( sigqueue(pid, SIGSTOP, sinfo) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CSyscallState::_OnSetpgidReturn() - Error sending SIGSTOP: " << errMsg;
        throw ex;
    }

    m_insertedSyscall = SYS_mmap;
}

void CSyscallState::_OnMmapReturn(pid_t pid, userreg_t status)
{
    using namespace log4cpp;

    // The return value mmap() call is the start address of the allocated memory stored in the rax register.
    userreg_t pageStart = status; 
    m_intercept.SetPageStart(pageStart);

    m_logger.noticeStream() << '[' << pid << "] CSyscallState::_OnMmapReturn() - Address of reserved page " << (void*)pageStart << eol; 
    m_logger.noticeStream().flush();
   
    m_insertedSyscall = NOSYSCALL;
}
