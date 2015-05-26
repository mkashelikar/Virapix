#include "Global.h"
#include "CForkState.h"
#include "CProcCtrl.h"
#include "CException.h"

#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string>

#include <log4cpp/Category.hh>

inline int CForkState::_Wait()
{
    return m_pProcCtrl->_Wait();
}

CForkState::CForkState(CProcCtrl* pProcCtrl) : CState(pProcCtrl)
{
}

CForkState::~CForkState()
{
}

void CForkState::Execute(pid_t pid, int sigNum)
{
    using namespace log4cpp;
    using namespace std;

    long childPid = 0;

    if ( ptrace(PTRACE_GETEVENTMSG, pid, 0, &childPid) == -1 ) 
    {
        CException ex(errno);
        ex << '[' << pid << "] CForkState::Execute() - PTRACE_GETEVENTMSG - " << errMsg;
        throw ex;
    }

    m_logger.noticeStream() << "\n\t\t\t*****************************************************************\n" << eol;
    m_logger.noticeStream() << '[' << pid << "] CForkState::Execute() - New process {" << childPid << "} (v)forked." << eol; 
    m_logger.noticeStream().flush();

    _OnFork(pid, childPid);

    if ( ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1 )
    {
        CException ex(errno);
        ex << '[' << childPid << "] CForkState::Execute() - PTRACE_SYSCALL - " << errMsg;
        throw ex;
    }
}

void CForkState::Notify(int code, int data)
{
}

void CForkState::_OnFork(pid_t pid, pid_t childPid)
{
    // Please note:
    // pid refers to the entity (a process or one of its threads) that fork()ed/vfork()ed a process.
    // abs(m_pProcCtrl->m_pid) ***always*** refers to the group leader, i.e. the top most process in this group.
    // childPid is the pid of the newly forked process. 
    using namespace log4cpp;
    using namespace std;

    // Spawn a new controller thread. pProcCtrl is managed by the application object.
    CProcCtrl* pProcCtrl = new CProcCtrl(childPid, m_pProcCtrl->_Is64Bit());
    pProcCtrl->Run();

    // We receive either SIGWAKEPARENT0 or SIGWAKEPARENT1.
    // Case 1: SIGWAKEPARENT0: If the newly forked process fails to attach to the controller thread, CProcCtrl sends us this signal.
    // Case 2: SIGWAKEPARENT1: Implies successful attach. Now, the newly forked process belongs in the parent's group. CStopState immediately 
    // instructs it to lead a new group. Till that happens, it's the parent process that will receive the child's events. Remember the negative 
    // value of m_pProcCtrl->m_pid passed to waitpid() earlier. To avoid this, the parent pauses and waits for the child to change its process group
    // which is communicated to us by CSyscallState through this signal.
    if ( _Wait() == SIGWAKEPARENT0 )
    {
        m_logger.errorStream() << '[' << abs(m_pProcCtrl->m_pid) << "] Child {" << childPid << "} either failed to attach or no longer exists."
           << eol;
        m_logger.errorStream().flush();
        return;
    }

    m_logger.noticeStream() << '[' << abs(m_pProcCtrl->m_pid) << "] CForkState::_OnFork() - Signalled by the child controller thread. Resuming "
        "execution." << eol;
    m_logger.noticeStream().flush();
}
