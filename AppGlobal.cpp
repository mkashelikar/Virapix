#include "Defs.h"
#include "AppDefs.h"
#include "AppGlobal.h"
#include "Global.h"
#include "CElfReader.h"
#include "SConfig.h"
#include "CException.h"

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fstream>

namespace
{
    void Assign(char* name, char* val, SConfig& config, const std::string& rootDir);
    log4cpp::Priority::PriorityLevel LogLevel(char* str);

    inline long LogSize(long logSize)
    {
        return (logSize > CFG_MAXLOGSIZE || logSize < CFG_MINLOGSIZE) ? CFG_DEFLOGSIZE : logSize;
    }

    inline short LogRollover(int rollover)
    {
        return (rollover > CFG_MAXROLLOVER || rollover < CFG_MINROLLOVER) ? CFG_DEFROLLOVER : rollover;
    }
}

void SetSigProcMask()
{
    sigset_t sigMask;
    sigemptyset(&sigMask);
    sigaddset(&sigMask, SIGWAKEPARENT0);
    sigaddset(&sigMask, SIGWAKEPARENT1);
#ifdef __x86_64__
    sigaddset(&sigMask, SIGUSR1);
#endif
    pthread_sigmask(SIG_SETMASK, &sigMask, NULL);
}

#ifdef __x86_64__
void WaitTillReady(pid_t pid)
{
    siginfo_t sigInfo;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    while ( true )
    {
        sigwaitinfo(&set, &sigInfo);

        if ( sigInfo.si_pid == pid && sigInfo.si_signo == SIGUSR1 )
        {
            break;
        }
    }
}
#endif

unsigned long GetMemOffset(const std::string& symbol, const std::string& lib)
{
    unsigned long symOffset = 0ul;

    CElfReader reader;

    reader.Open(lib);
    symOffset = reader.GetMemOffset(symbol);

    return symOffset;
}

void GetNewRootDir(std::string& newRootDir)
{
    using namespace std;

    string selfPath;

    GetExePath(getpid(), selfPath);

    int pos = selfPath.find(APP_DIR);

    if ( pos != selfPath.npos )
    {
        newRootDir = selfPath.substr(0, pos - 1);
    }
    else
    {
        throw CException(ERR_NOEXEPATH);
    }
}

void ReadCfg(SConfig& config, const std::string& rootDir)
{
    using namespace std;
    using namespace log4cpp;

    config.m_logFile = rootDir + '/' + CFG_DEFLOGFILE;
    config.m_logLevel = Priority::WARN;
    config.m_logSize = CFG_DEFLOGSIZE;
    config.m_logRollover = CFG_DEFROLLOVER;

    string configFile = rootDir + '/' + APP_CFG;

    ifstream inFile(configFile.c_str());

    if ( !inFile )
    {
        return;
    }

    char buf[CFG_MAXRECLEN + 1];
    char* name = 0;
    char* val = 0;

    while ( true )
    {
        if ( !inFile.getline(buf, CFG_MAXRECLEN + 1) )
        {
            break;
        }

        Trim(buf);

        if ( *buf && *buf != '#' )
        {
            name = strtok(buf, "#=");
            val = strtok(0, "#=");

            RTrim(name);
            Trim(val);
            
            Assign(name, val, config, rootDir);
        }
    }
}

namespace
{

void Assign(char* name, char* val, SConfig& config, const std::string& rootDir)
{
    if ( !name || !*name || !val || !*val )
    {
        return;
    }

    if ( !strcasecmp(name, PARAM_LOGFILE) )
    {
        config.m_logFile = rootDir + '/' + val;
    }
    else if ( !strcasecmp(name, PARAM_LOGLEVEL) )
    {
        config.m_logLevel = LogLevel(val);
    }
    if ( !strcasecmp(name, PARAM_LOGSIZE) )
    {
        config.m_logSize = LogSize(atol(val));
    }
    if ( !strcasecmp(name, PARAM_LOGROLLOVER) )
    {
        config.m_logRollover = LogRollover(atoi(val)) - 1; // Log4cpp uses zero-based index.
    }
}

log4cpp::Priority::PriorityLevel LogLevel(char* str)
{
    using namespace log4cpp;

    Priority::PriorityLevel retval = Priority::WARN;

    if ( !strcasecmp(str, LOGLEVEL_DBG) )
    {
        retval = Priority::DEBUG;
    }
    else if ( !strcasecmp(str, LOGLEVEL_INFO) )
    {
        retval = Priority::INFO;
    }
    else if ( !strcasecmp(str, LOGLEVEL_NOTICE) )
    {
        retval = Priority::NOTICE;
    }
    else if ( !strcasecmp(str, LOGLEVEL_WARN) )
    {
        retval = Priority::WARN;
    }
    else if ( !strcasecmp(str, LOGLEVEL_ERR) )
    {
        retval = Priority::ERROR;
    }
    else if ( !strcasecmp(str, LOGLEVEL_CRIT) )
    {
        retval = Priority::CRIT;
    }
    else if ( !strcasecmp(str, LOGLEVEL_FATAL) )
    {
        retval = Priority::FATAL;
    }

    return retval;
}

}
