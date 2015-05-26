#include "Global.h"
#include "ProcUtil.h"
#include "SyscallGlobal.h"
#include "CExecState.h"
#include "CProcCtrl.h"
#include "CException.h"

#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string>

#include <log4cpp/Category.hh>

CExecState::CExecState(CProcCtrl* pProcCtrl) : CState(pProcCtrl)
{
}

CExecState::~CExecState()
{
}

void CExecState::Execute(pid_t pid, int)
{
    using namespace log4cpp;
    using namespace std;

    int bitness = GetBitness(pid);
    m_pProcCtrl->_Is64Bit(bitness == 64);

    // We are signalling the controlled process (pid) in preparation for setting up a memory page for our internal use. CStopState processes the
    // resultant SIGSTOP and inserts mmap() in target address space. We might as well do that here itself and speed up things. Unfortunately doing 
    // so gives unpredictable results (may be because execve() is yet to return?).
    sigval sinfo;
    sinfo.sival_int = SYS_mmap;

    if ( sigqueue(pid, SIGSTOP, sinfo) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CExecState::Execute() - sigqueue(SIGSTOP) - " << errMsg;
        throw ex;
    }

    string str;

    m_logger.noticeStream() << '[' << pid << "] CExecState::Execute() - New program {" << GetExePath(pid, str) << "} [" << bitness
        << " bit] execved." << eol;
    m_logger.noticeStream().flush();

    if ( ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CExecState::Execute() - PTRACE_SYSCALL - " << errMsg;
        throw ex;
    }
}

void CExecState::Notify(int code, int data)
{
}
