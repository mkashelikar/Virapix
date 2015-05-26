#include "Defs.h"
#include "Global.h"
#include "AppDefs.h"
#include "AppGlobal.h"
#include "ProcUtil.h"
#include "CApp.h"
#include "CProcCtrl.h"
#include "SConfig.h"
#include "CException.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/Category.hh>

CApp* CApp::m_pSelf = 0;

CApp::CApp(int argc, char** argv, char** envp) : m_argc(argc), m_argv(argv), m_envp(envp), m_ok(false), m_mmapOffset64(0ul),
    m_setpgidOffset64(0ul), m_mmapOffset32(0ul), m_setpgidOffset32(0ul), m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{
    using namespace log4cpp;
    using namespace std;

    _InstallSigHandler();

    try
    {
        ::GetNewRootDir(m_newRootDir);
        chdir(m_newRootDir.c_str());

        SConfig config;
        ::ReadCfg(config, m_newRootDir);
        _InitLogger(config);

        SMemMap memMap;
        
        GetMemMap(getpid(), E_LDSO, memMap);

        #ifdef __x86_64__
        m_mmapOffset64 = GetMemOffset("mmap", memMap.m_path);
        #else
        m_mmapOffset32 = GetMemOffset("mmap", memMap.m_path);
        #endif
                         
        m_ldso = basename(memMap.m_path.c_str());

        GetMemMap(getpid(), E_LIBC, memMap);

        #ifdef __x86_64__
        m_setpgidOffset64 = GetMemOffset("setpgid", memMap.m_path);
        #else
        m_setpgidOffset32 = GetMemOffset("setpgid", memMap.m_path);
        #endif
                            
        m_libc = basename(memMap.m_path.c_str());

        // Set the signal mask which will later be inherited by our threads.
        SetSigProcMask();

#ifdef __x86_64__
        _Get32BitOffset(); // For 32 bit applications
#endif

        // Not absolutely essential if our startup script itself sets these variables.
        // _SetEnv();

        m_ok = true;
    }

    catch ( const CException& ex )
    {
        m_logger.fatalStream() << '[' << getpid() << "] CApp::CApp() - " << ex.str() << eol;
        m_logger.fatalStream().flush();
    }
}

// Dummy private copy constructors only to prevent copying of CApp objects. Never used.
CApp::CApp(const CApp&) : m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{
}

CApp::CApp(const CApp*) : m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{
}

CApp::~CApp()
{
}

CApp* CApp::GetInstance(int argc, char** argv, char** envp)
{
    return m_pSelf ? m_pSelf : (m_pSelf = new CApp(argc, argv, envp));
}

void CApp::Destroy()
{
    delete m_pSelf;
    m_pSelf = 0;
}

void CApp::Run()
{
    using namespace log4cpp;
    using namespace std;

    try
    {
        _Run();
        m_logger.noticeStream() << "\n\t\t********************************* END! ********************************" << eol;
        m_logger.noticeStream().flush();
    }

    catch ( const CException& ex )
    {
        m_logger.fatalStream() << string(ex) << eol;
        m_logger.fatalStream().flush();
    }

    catch ( ... )
    {
        m_logger.fatalStream() << '[' << getpid() << "] CApp::Run() - Unknown exception thrown." << eol;
        m_logger.fatalStream().flush();
    }

    return;
}

void CApp::_SigHandler(int sigNum, siginfo_t* pSigInfo, void* pDummy)
{
    if ( sigNum == SIGREGISTER )
    {
        m_pSelf->_RegisterICtrl(pSigInfo);
    }
}

void CApp::_RegisterICtrl(siginfo_t* pSigInfo)
{
    using namespace log4cpp;

    if ( pSigInfo->si_code == SI_QUEUE && pSigInfo->si_pid == getpid() )
    {
        ICtrl* pICtrl = static_cast<ICtrl*> (pSigInfo->si_value.sival_ptr);
        m_iCtrlList.push_back(pICtrl);

        // It's safe to call write() and lseek() from within a handler as per POSIX.1-2008. These two are the only system calls
        // resulting from the next two lines.
        m_logger.noticeStream() << '[' << pICtrl->GetPid() << "] CApp::_RegisterICtrl() - Successfully registered ICtrl object!" << eol; 
        m_logger.noticeStream().flush();
    }
}

void CApp::_InstallSigHandler()
{
    struct sigaction sigAction;
    sigset_t set;

    memset(&sigAction, 0, sizeof sigAction);
    sigemptyset(&set);

    sigAction.sa_sigaction = CApp::_SigHandler;
    sigAction.sa_mask = set;
    sigAction.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGREGISTER, &sigAction, 0);
}

void CApp::_InitLogger(const SConfig& config)
{
    using namespace log4cpp;

    // This is how we initialise lo4cpp logger. The appender class determines the destination of the logs. There can be multiple
    // appenders attached to a logger, but we need only one that is associated with the disk file LOG_FILE. The Layout object 
    // controls the format of the output. The member object m_logger of type Category is the actual logging entity.

    // RollingFileAppender writes a new file once the existing one reaches the size LOG_FILE_SIZE.
    Appender* pAppender = new RollingFileAppender(LOG_APPENDER, config.m_logFile.c_str(), config.m_logSize, config.m_logRollover, false);
    Layout* pLayout = new SimpleLayout(); // Bare bones format. We don't need elaborate details at this moment.
    pAppender->setLayout(pLayout);
    m_logger.setAdditivity(false); // Ensures that one and only one appender is attached to the logger.
    m_logger.setAppender(pAppender); // Thanks to setAdditivity(), pAppender replaces (instead of getting appended) any existing
                                     // appender object.
    m_logger.setPriority(config.m_logLevel);
}

void CApp::_SetEnv()
{
    using namespace log4cpp;
    using namespace std;

    const char* HOME = "HOME";
    const char* XAUTHORITY = "XAUTHORITY";

    const char* strHomeDir = getenv(HOME);

    if ( !strHomeDir )
    {
        CException ex;
        ex << ERR_NOHOMEVAR;
        throw ex;
    }

    string homeDir = strHomeDir;

    if ( homeDir.find(m_newRootDir) ) // A non-zero value means that $HOME doesn't refer to new root dir.
    { 
        homeDir = m_newRootDir + homeDir;
        setenv(HOME, homeDir.c_str(), 1);

        m_logger.infoStream() << '[' << getpid() << "] CApp::CApp() - $HOME set to " << getenv(HOME) << eol;
        m_logger.infoStream().flush();
    }

    const char* strXauth = getenv(XAUTHORITY);
    string xauthority;

    if ( !strXauth )
    {
        xauthority = homeDir + '/' + ".Xauthority";
        setenv(XAUTHORITY, xauthority.c_str(), 1);

        m_logger.infoStream() << '[' << getpid() << "] CApp::CApp() - $XAUTHORITY set to " << getenv(XAUTHORITY) << eol;
        m_logger.infoStream().flush();
    }
    else 
    {
        xauthority = strXauth;

        if ( xauthority.find(homeDir) ) // See comment above.
        {
            xauthority = homeDir + '/' + ".Xauthority";
            setenv(XAUTHORITY, xauthority.c_str(), 1);

            m_logger.infoStream() << '[' << getpid() << "] CApp::CApp() - $XAUTHORITY set to " << getenv(XAUTHORITY) << eol;
            m_logger.infoStream().flush();
        }
    }
}

#ifdef __x86_64__
void CApp::_Get32BitOffset()
{
    using namespace std;

    pid_t childPid = 0;
    
    if ( ( childPid = fork() ) == -1 )
    {
        CException ex(errno);
        ex << '[' << getpid() << "] CApp::_Get32BitOffset() - " << errMsg;
        throw ex;
    }
    else
    {
        if ( childPid ) // Parent
        {
            WaitTillReady(childPid);

            SMemMap memMap;
            GetMemMap(childPid, E_LDSO, memMap);
            m_mmapOffset32 = GetMemOffset("mmap", memMap.m_path);

            GetMemMap(childPid, E_LIBC, memMap);
            m_setpgidOffset32 = GetMemOffset("setpgid", memMap.m_path);

            kill(childPid, SIGUSR1);
            waitpid(childPid, 0, 0);
        }
        else // Child
        {
            string app32 = m_newRootDir + '/' + APP_DUMMY32;
            char* const arg[] = {(char*)app32.c_str(), 0};

            execve(app32.c_str(), arg, 0);

            // We reach here only if execve() fails
            CException ex(errno);
            ex << '[' << getpid() << "] CApp::_Get32BitOffset() - " << errMsg;
            throw ex;
        }
    }
}
#endif

void CApp::_Run()
{
    using namespace log4cpp;

    if ( !m_ok )
    {
        m_logger.fatalStream() << '[' << getpid() << "] CApp::_Run() - Object not initialised." << eol;
        m_logger.fatalStream().flush();
        return;
    }

    pid_t childPid = 0;
    
    if ( ( childPid = fork() ) == -1 )
    {
        std::string str;
        m_logger.fatalStream() << '[' << getpid() << "] CApp::_Run() - " << GetLastError(str) << eol;
        m_logger.fatalStream().flush();
    }
    else
    {
        if ( childPid ) // Parent
        {
            // Put the child in its own process group.
            setpgid(childPid, 0);

            m_logger.noticeStream() << '[' << childPid << "] CApp::_Run() - First child forked. Process group id " << getpgid(childPid)
                << eol;
            m_logger.noticeStream().flush();

            // The last argument tells CProcCtrl not to register with CApp since we are tracking this process
            // separately (take a look at the invocation of _WaitForCompletion()).
            ICtrl* pICtrl = new CProcCtrl(childPid, WORD_SIZE == 8, false);
            pICtrl->Run();

            _WaitForCompletion(pICtrl);
        }
        else // Child
        {
            if ( ptrace(PTRACE_TRACEME, 0, 0, 0) == -1 )
            {
                CException ex(errno);
                ex << '[' << getpid() << "] CApp::_Run() - " << errMsg;
                throw ex;
            }

            // PTRACE_TRACEME doesn't arrange for any signal to be delivered to us when we enter execve(). We raise SIGSTOP ourselves 
            raise(SIGSTOP);
            execve(m_argv[1], &m_argv[1], m_envp);

            // We reach here only if execve() fails
            CException ex(errno);
            ex << '[' << getpid() << "] CApp::_Run() - " << errMsg;
            throw ex;
        }
    }
}

void CApp::_WaitForCompletion(ICtrl* pTop)
{
    using namespace log4cpp;
    using namespace std;

    // This is the topmost process (typically a window manager) which is waited for separately for the following reason: we 
    // (the virtual machine) are the leader of the new X session that the user initiates (usually through a call to startx) 
    // and as such effectively control its lifetime. The session usually includes daemon processes that are not under direct 
    // control of the window manager and which are shut down only when the session itself is destroyed. In the absence of a 
    // virtual machine, termination of the window manager (eg. when the user clicks 'Exit') terminates the session as well
    // and along with it all the daemon processes. However when we are in charge, exiting from the window manager does
    // ***not*** kill the session because it's we who are the session leader. This also means that the daemons are still around
    // and unless they are shot down explicitly, the virtual machine and consequently the session itself will remain alive.
    // So what we do is wait for the window manager and then destroy the surviving child processes if any.
    pTop->Wait();

    m_logger.noticeStream() << '[' << pTop->GetPid() << "] CApp::_WaitForCompletion() - The leader process has shut down." << eol;
    m_logger.noticeStream() << "\n\t\t**************************************************************\n" << eol;
    m_logger.noticeStream().flush();

    int status = 0;
    std::string str;

    for ( ICtrlList::iterator iter = m_iCtrlList.begin(); iter != m_iCtrlList.end(); 
          iter = m_iCtrlList.erase(iter) )
    {
        if ( status = (*iter)->Terminate() )
        {
            m_logger.errorStream() << '[' << (*iter)->GetPid() << "] CApp::_WaitForCompletion() - " << GetLastError(str, status) 
                << eol;
            m_logger.errorStream().flush();
        }

        delete *iter;
    }

    delete pTop;
}
