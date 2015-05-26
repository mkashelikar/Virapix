#include "Defs.h"
#include "Global.h"
#include "ProcCtrlGlobal.h"
#include "CProcCtrl.h"
#include "CException.h"
#include "StateDefs.h"
#include "CDefState.h"
#include "CStopState.h"
#include "CSyscallState.h"
#include "CForkState.h"
#include "CCloneState.h"
#include "CExecState.h"

#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include <log4cpp/Category.hh>

CProcCtrl::CProcCtrl(pid_t pid, bool is64Bit, bool registerSelf) : m_flags(0), m_pid(pid), m_thread(this), 
    m_pCurState(0), m_pRegsBackup(0), m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{
#ifdef __x86_64__
    m_flags |= (is64Bit ? F_PROC64BIT : 0);
#endif
    _CreateStates();

    if ( registerSelf )
    {
        _RegisterSelf();
    }

    _PrepareAttach();

    // The first child. The application object gives it its own process group and as required by waitpid() (Read the 
    // comments in _Run()), we negate the value.
    if ( getpgid(pid) == pid )
    {
        m_pid = -pid;
    }
}

CProcCtrl::~CProcCtrl()
{
    _DestroyStates();
}

void CProcCtrl::Run()
{
    if ( m_flags & F_PROCOK )
    {
        m_thread.Run();
    }
}

int CProcCtrl::Wait()
{
    return ( m_flags & F_PROCOK ) ? m_thread.Join() : ECHILD;
}

int CProcCtrl::Terminate()
{
    if ( m_thread.IsRunning() )
    {
        kill(abs(m_pid), SIGSHUTDOWN);
    }

    return Wait();
}

/////////////////////////////// Private methods ////////////////////////////////////////

inline void CProcCtrl::_SetCurState(bool isStopped, int waitStatus)
{
    // The first seven bits of the second byte of waitStatus contain the signal number; its eighth bit is set for syscall enter/exit.
    // For process/program creation (clone(), fork(), vfork() and execve()), the eighth bit is cleared, but the first few bits of the
    // ***third*** byte are suitably set. Because we are not interested in the first byte, we right shift by 8 and return the signal 
    // number plus the extra bits. 
    int sig = GetPtraceSig(waitStatus);
    m_pCurState = isStopped ? ( m_states.find(sig) != m_states.end() ? m_states[sig] : m_states[0] ) : m_states[0];
}

inline void CProcCtrl::_Execute(pid_t pid, int sigNum)
{
    m_pCurState->Execute(pid, sigNum);
}

void CProcCtrl::_CreateStates()
{
    m_states[IDX_DEFSTATE] = new CDefState(this);
    m_states[IDX_STOPSTATE] = new CStopState(this);
    m_states[IDX_SYSCALLSTATE] = new CSyscallState(this);
    m_states[IDX_FORKSTATE] = new CForkState(this);
    m_states[IDX_VFORKSTATE] = new CForkState(this);
    m_states[IDX_CLONESTATE] = new CCloneState(this);
    m_states[IDX_EXECSTATE] = new CExecState(this);
}

void CProcCtrl::_RegisterSelf()
{
    sigval sinfo;
    sinfo.sival_ptr = this;

    // Send a signal to self (i.e. the running process) so that any of the constituent threads will register us (CProcCtrl 
    // object) with the main application object. We use real-time signal because (unlike normal signals), more than one of them can be
    // queued to the target process. Besides, POSIX guarantees that a signal won't interrupt the handler servicing an earlier
    // instance of the same signal. This ensures synchronised access to the application data.
    sigqueue(getpid(), SIGREGISTER, sinfo);
}

void CProcCtrl::_PrepareAttach()
{
    // At this juncture, we are in the parent which has just begun tracing a process. To enable ptrace events in the 
    // soon-to-be-created thread, it's necessary to first detach the target from the tracing parent. ptrace() doesn't allow multiple
    // threads to trace an entity even if they are in the same process.
    
    using namespace log4cpp;

    int waitStatus = 0;

    waitpid(m_pid, &waitStatus, __WALL);

    if ( IsAlive(waitStatus) )
    {
        // Stop the target after detaching it from the parent. This enables our thread to control it early before it invokes any
        // more system calls.
        if ( ptrace(PTRACE_DETACH, abs(m_pid), 0, SIGSTOP) == -1 )
        {
            std::string str;
            m_logger.fatalStream() << '[' << abs(m_pid) << "] CProcCtrl::_PrepareAttach() - PTRACE_DETACH - " << GetLastError(str) 
                << eol;
            m_logger.fatalStream().flush();

            _KillProcess();
        }
        else
        {
            m_flags |= F_PROCOK;
            m_logger.noticeStream() << '[' << abs(m_pid) << "] CProcCtrl::_PrepareAttach() - Detached from controller thread {" 
                << syscall(SYS_gettid) <<  '}' << eol;
            m_logger.noticeStream().flush();
        }
    }
    else
    {
        m_logger.critStream() << '[' << abs(m_pid) << "] CProcCtrl::_PrepareAttach() - Process in an indeterminate state." << eol;
        m_logger.critStream().flush();
        
        _KillProcess();
    }
}

void CProcCtrl::_Attach()
{
    using namespace log4cpp;

    if ( ptrace(PTRACE_ATTACH, abs(m_pid), 0, 0) == -1 )
    {
        std::string str;
        m_logger.fatalStream() << '[' << abs(m_pid) << "] CProcCtrl::_Run() - PTRACE_ATTACH - " << GetLastError(str) << eol;
        m_logger.fatalStream().flush();

        _KillProcess();

        // IMPORTANT! Communicate failure to our creator thread.
        _NotifyParent(SIGWAKEPARENT0);
    }
    else
    {
        m_logger.noticeStream() << '[' << abs(m_pid) << "] CProcCtrl::_Run() - Attached to new controller thread {" 
            << syscall(SYS_gettid) << '}' << eol;
        m_logger.noticeStream().flush();
    }
}

void* CProcCtrl::_Start(void* unused)
{
    using namespace log4cpp;

    int waitStatus = 0;
    int sigNum = 0;
    bool processEnd = false;
    pid_t pid = 0;

    _Attach();

    do
    {
        // If m_pid is positive, this won't allow us to receive events from threads. A pid passed to waitpid() as the first
        // parameter will restrict us only to a ***single*** process with that pid. Although getpid() called in a thread will 
        // return the pid of the containing process, a thread being not much different from a process (in Linux) is essentially a 
        // child process with its own tid (i.e. pid) and which lives in the same process group as that of its parent. To receive
        // events from threads in our process group, we therefore pass a negative value of pid. However, this poses a new problem:
        // newly forked processes belong in this group too and we want to control each of them separately, not here. Thus a child
        // needs to have its own process group. We achieve this by inserting a call to setpgid() in the child (later in 
        // _OnProcCreate()). This done, we negate m_pid (in _OnSyscall()) and wait on it instead.

        if ( (pid = waitpid(m_pid, &waitStatus, __WALL)) != -1 )
        {
            // Ensure that pid hasn't exited (normally) nor is it terminated by SIGKILL nor has it dumped core. Note that SIGKILL is 
            // delivered to pid irrespective of whether or not it is traced.
            if ( IsAlive(waitStatus) )
            {
                sigNum = WIFSTOPPED(waitStatus) ? WSTOPSIG(waitStatus) : 0;
                _SetCurState(!!sigNum, waitStatus);

                try
                {
                    _Execute(pid, sigNum);
                }

                catch ( const CException& ex )
                {
                    m_logger.fatalStream() << std::string(ex) << eol;
                    m_logger.fatalStream().flush();

                    // If m_pid is negative, the signal shall be sent to all the entities in that group. The zombie will be cleared when we loop.
                    kill(m_pid, SIGKILL);
                }
            }
            else // Process/thread no longer executing...
            {
                // Remember we are waiting on process group, not on just one process. Consequently, if waitpid() returns an id other than
                // abs(m_pid), then the id is that of one of the constituent threads.
                processEnd = (pid == abs(m_pid));
                m_logger.noticeStream() << '[' << pid << "] CProcCtrl::_Start() - " << (processEnd ? "Process" : "Thread") << " ended." << eol; 
                m_logger.noticeStream().flush();
            }
        }
        else // waitpid() returns -1 also if it's interrupted by a signal...
        {
            processEnd = (errno == ECHILD);
        }
    } while ( !processEnd );

    if ( m_flags & F_SYSSETPGID )
    {
        // If the above flag is set, it means that something went wrong when the process was executing setpgid() that we had inserted earlier. To 
        // unblock the waiting creator thread (See CForkState::_OnFork()), we send it a suitable signal.
        _NotifyParent(SIGWAKEPARENT0);
    }

    return 0;
}

void CProcCtrl::_PreInsertSyscall(int syscall)
{
    if ( syscall == SYS_setpgid )
    {
        m_flags |= F_SYSSETPGID;
    }
}

void CProcCtrl::_SyscallInserted(int syscall)
{
    m_states[IDX_SYSCALLSTATE]->Notify(NOTIFY_SYSINSERT, syscall);
}

void CProcCtrl::_SyscallReturn(int syscall)
{
    if ( syscall == SYS_setpgid )
    {
        m_flags &= ~F_SYSSETPGID;
    }
}

void CProcCtrl::_KillProcess()
{
    // If m_pid is negative, the signal shall be sent to all the processes in that group.
    kill(m_pid, SIGKILL);

    int pid = 0;
    int waitErrno = 0;

    // Get rid of the zombie(s)
    do
    {
        if ( (pid = waitpid(m_pid, 0, __WALL)) == -1 )
        {
            waitErrno = errno;
        }
    } while ( waitErrno != ECHILD );
}

void CProcCtrl::_DestroyStates()
{
    using namespace std;

    for ( map<int, IState*>::iterator iter = m_states.begin(); iter != m_states.end(); )
    {
        delete iter->second;
        m_states.erase(iter++);
    }
}
