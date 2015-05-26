#ifndef _SConfig_
#define _SConfig_

#include <string>
#include <log4cpp/Priority.hh>

struct SConfig
{
    std::string m_logFile;
    log4cpp::Priority::PriorityLevel m_logLevel;
    long m_logSize;
    short m_logRollover;
};

#endif
