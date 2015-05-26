#include "Defs.h"
#include "Global.h"
#include "CDefState.h"
#include "CException.h"

#include <sys/ptrace.h>
#include <signal.h>
#include <errno.h>
#include <string>

#include <log4cpp/Category.hh>

extern char* sigName[];

CDefState::CDefState(CProcCtrl* pProcCtrl) : CState(pProcCtrl)
{
}

CDefState::~CDefState()
{
}

void CDefState::Execute(pid_t pid, int sigNum)
{
    using namespace log4cpp;

    // This signal is sent to the process by us during shut down.
    if ( sigNum == SIGSHUTDOWN )
    {
        siginfo_t sigInfo;

        if ( ptrace(PTRACE_GETSIGINFO, pid, 0, &sigInfo) == -1 )
        {
            CException ex(errno);
            ex << '[' << pid << "] CDefState::Execute() - PTRACE_SYSCALL - " << errMsg;
            throw ex;
        } 

        if ( sigInfo.si_pid == getpid() ) // Verify that we are the sender
        {
            m_logger.noticeStream() << '[' << pid << "] CDefState::Execute() - Application shutdown: terminating process..." 
                << eol; 
            m_logger.noticeStream().flush();

            ptrace(PTRACE_KILL, pid, 0, 0);
        }
    }
    else
    {
        m_logger.noticeStream() << '[' << pid << "] CDefState::Execute() - Received signal " << sigName[sigNum] << eol; 
        m_logger.noticeStream().flush();

        // We get spurious SIGTRAP once in a while: ignore them.
        if ( ptrace(PTRACE_SYSCALL, pid, 0, (sigNum != SIGTRAP ? sigNum : 0)) == -1 )
        {
            CException ex(errno);
            ex << '[' << pid << "] CDefState::Execute() - PTRACE_SYSCALL - " << errMsg;
            throw ex;
        }
    }
}

void CDefState::Notify(int code, int data)
{
}
