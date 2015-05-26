#ifndef _SIParam_
#define _SIParam_

#include <sys/types.h>

class CIntercept;
struct user_regs_struct;

struct SIParam
{
    CIntercept& m_intercept;
    pid_t m_pid;
    user_regs_struct& m_userRegs;
    int m_flags;

    SIParam(CIntercept& intercept, pid_t pid, user_regs_struct& userRegs, int flags) : m_intercept(intercept), m_pid(pid), 
        m_userRegs(userRegs), m_flags(flags)
    {
    }
};

#endif
