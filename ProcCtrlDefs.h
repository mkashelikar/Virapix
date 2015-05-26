#ifndef _ProcCtrlDefs_
#define _ProcCtrlDefs_

#include <signal.h>
#include <sys/ptrace.h>

const int SIGSYSCALL = SIGTRAP | 0x80;

const int IDX_DEFSTATE = 0;
const int IDX_STOPSTATE = SIGSTOP;
const int IDX_SYSCALLSTATE = SIGSYSCALL;
const int IDX_FORKSTATE = SIGTRAP | (PTRACE_EVENT_FORK << 8);
const int IDX_VFORKSTATE = SIGTRAP | (PTRACE_EVENT_VFORK << 8);
const int IDX_CLONESTATE = SIGTRAP | (PTRACE_EVENT_CLONE << 8);
const int IDX_EXECSTATE = SIGTRAP | (PTRACE_EVENT_EXEC << 8);

const int F_PROCOK = 1;
const int F_PROC64BIT = 2;
const int F_SYSSETPGID = 4;

#endif

