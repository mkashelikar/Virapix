#ifndef _CSysCompat_
#define _CSysCompat_

#ifdef __x86_64__

#include "SyscallTypeDefs.h"
#include <ext/hash_map>

namespace sys32
{
    // socketcall() is not defined for x86_64. We create a phantom constant to map to 32 bit SYS_socket.
    const int SYS_SOCKETCALL = 777;
}

#endif

struct user_regs_struct;

userreg_t GetSyscallNumXlat(user_regs_struct& userRegs, bool* ignore);
void SetSyscallNumXlat(userreg_t syscall, user_regs_struct& userRegs);

class CSysCompat
{
    friend userreg_t GetSyscallNumXlat(user_regs_struct& userRegs, bool* ignore);
    friend void SetSyscallNumXlat(userreg_t syscall, user_regs_struct& userRegs);

    public:
        static CSysCompat* GetInstance();
        static void Destroy();

    private:
        CSysCompat();
        ~CSysCompat() { };

    private:
#ifdef __x86_64__
        static CSysCompat* m_pSelf;
        __gnu_cxx::hash_map<int, int> m_sys32To64;
        __gnu_cxx::hash_map<int, int> m_sys64To32;

        int _Sys32To64(int syscall32, bool& isOk);
        int _Sys64To32(int syscall64);
        void _MapSyscall32To64();
        void _MapSyscall64To32();
#endif
};

#ifdef __x86_64__

// Here we map those calls that we intercept and optionally modify, i.e. syscall numbers that we "read" from ptrace().
inline int CSysCompat::_Sys32To64(int syscall32, bool& isOk)
{
    isOk = false;
    int retval = syscall32;
    __gnu_cxx::hash_map<int, int>::iterator iter = m_sys32To64.find(syscall32);

    if ( iter != m_sys32To64.end() )
    {
        retval = iter->second;
        isOk = true;
    }

    return retval;
}

// Here we map syscall numbers that we "send" to ptrace().
inline int CSysCompat::_Sys64To32(int syscall64)
{
    __gnu_cxx::hash_map<int, int>::iterator iter = m_sys64To32.find(syscall64);
    return iter != m_sys64To32.end() ? iter->second : syscall64;
}

#endif

#endif
