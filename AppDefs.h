#ifndef _AppDefs_
#define _AppDefs_

const char APP_DIR[] = "virapix";
const char APP_DUMMY32[] = "virapix/bin/dummy32";
const char APP_CFG[] = "virapix/cfg/.virapixrc";

const char PARAM_LOGFILE[] = "logfile";
const char PARAM_LOGLEVEL[] = "loglevel";
const char PARAM_LOGSIZE[] = "logsize";
const char PARAM_LOGROLLOVER[] = "logrollover";

const char LOGLEVEL_DBG[] = "debug";
const char LOGLEVEL_INFO[] = "info";
const char LOGLEVEL_NOTICE[] = "notice";
const char LOGLEVEL_WARN[] = "warning";
const char LOGLEVEL_ERR[] = "error";
const char LOGLEVEL_CRIT[] = "critical";
const char LOGLEVEL_FATAL[] = "fatal";

const int CFG_MAXRECLEN = 1024;
const char CFG_DEFLOGFILE[] = "virapix/log/virapix.log";
const long CFG_DEFLOGSIZE = 10485760L; // 10MB
const long CFG_MAXLOGSIZE = 52428800L; // 50 MB
const long CFG_MINLOGSIZE = 1048576L; // 1 MB
const long CFG_DEFROLLOVER = 5;
const long CFG_MAXROLLOVER = 100;
const long CFG_MINROLLOVER = 2;

const char LOG_APPENDER[] = "Avirapix";

const char ERR_NOEXEPATH[] = "Path of the executable not found";
const char ERR_NOHOMEVAR[] = "$HOME not set";

#endif
