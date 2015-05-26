#include "Global.h"
#include "CCloneState.h"
#include "CProcCtrl.h"
#include "CException.h"

#include <sys/ptrace.h>
#include <errno.h>
#include <string>

#include <log4cpp/Category.hh>

CCloneState::CCloneState(CProcCtrl* pProcCtrl) : CState(pProcCtrl)
{
}

CCloneState::~CCloneState()
{
}

void CCloneState::Execute(pid_t pid, int sigNum)
{
    using namespace log4cpp;
    using namespace std;

    long childPid;

    if ( ptrace(PTRACE_GETEVENTMSG, pid, 0, &childPid) == -1 ) 
    {
        CException ex(errno);
        ex << '[' << pid << "] CCloneState::Execute() - PTRACE_GETEVENTMSG - " << errMsg;
        throw ex;
    }

    m_logger.noticeStream() << '[' << pid << "] CCloneState::Execute() - New thread {" << childPid << "} created." << eol; 
    m_logger.noticeStream().flush();

    if ( ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CCloneState::Execute() - PTRACE_SYSCALL - " << errMsg;
        throw ex;
    }
}      

void CCloneState::Notify(int code, int data)
{
}
