#ifndef _SUserRegs_
#define _SUserRegs_

#include <sys/user.h>

#ifdef __x86_64__
struct SUserRegs : public user_regs_struct
{
    bool m_is64Bit;
};
#else
typedef user_regs_struct SUserRegs;
#endif

#endif
