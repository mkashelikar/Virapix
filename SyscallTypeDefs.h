#ifndef _SyscallTypeDefs_
#define _SyscallTypeDefs_

#ifdef __x86_64__
typedef unsigned long userreg_t;
#else
typedef long int userreg_t;
#endif

#endif
