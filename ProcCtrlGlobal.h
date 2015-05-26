#ifndef _ProcCtrlGlobal_
#define _ProcCtrlGlobal_

#include <sys/wait.h>

// HACK: Uses undocumented code. The first seven bits of the second byte of waitStatus contain the signal number; its eighth bit
// is set for syscall enter/exit. For process/program creation (clone(), fork(), vfork() and execve()), the eighth bit is cleared,
// but the first few bits of the ***third*** byte are suitably set. Because we are not interested in the first byte, we right shift
// by 8 and return the signal number plus the extra bits. 
inline int GetPtraceSig(int waitStatus)
{
    return (waitStatus >> 8) > 255 ? (waitStatus >> 8) : WSTOPSIG(waitStatus);
}

inline bool IsAlive(int waitStatus)
{
    return ( !WIFEXITED(waitStatus) && !WIFSIGNALED(waitStatus) && !WCOREDUMP(waitStatus) );
}

#endif
