#ifndef _Defs_
#define _Defs_

#include <sys/types.h>
#include <sys/user.h>
#include <signal.h>

const int ERRMSG_MAX_LEN = 256;
const int PATH_MAX_LEN = 256;
const int WORD_SIZE = sizeof (unsigned long);
const int RES_AREA_SIZE = PAGE_SIZE;
const int SIGREGISTER = SIGRTMIN + 5;
const int SIGWAKEPARENT0 = SIGRTMIN + 6;
const int SIGWAKEPARENT1 = SIGRTMIN + 7;
const int SIGSHUTDOWN = SIGRTMIN + 8;

const char LOG_CATEGORY[] = "Lvirapix";

#endif
