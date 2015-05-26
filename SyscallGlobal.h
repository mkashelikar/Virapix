#ifndef _SyscallGlobal_
#define _SyscallGlobal_

#include <sys/types.h>
#include <sys/reg.h>
#include <fcntl.h>
#include <string>

#include "Defs.h"
#include "SyscallTypeDefs.h"
#include "SyscallDefs.h"
#include "Global.h"
#include "SUserRegs.h"
#include "CSysCompat.h"

union UMemContent
{
    userreg_t m_val;
    char m_strVal[WORD_SIZE];
};

extern char* syscallName64[];
extern char* syscallName32[];

class CIntercept;

unsigned int GetNonNullCount(unsigned long word);
void SetSetpgidRegs64(user_regs_struct& newRegs, userreg_t setpgidAddress);
void SetSetpgidRegs32(pid_t pid, user_regs_struct& newRegs, userreg_t setpgidAddress);
void SetMmapRegs64(user_regs_struct& newRegs, userreg_t mmapAddress);
void SetMmapRegs32(pid_t pid, user_regs_struct& newRegs, userreg_t mmapAddress);
void PrintRegs(pid_t pid, user_regs_struct& userRegs, void* pProcCtrl, bool entryExit);
void ReadMemory(pid_t pid, userreg_t addr, char* buf);
void ReadMemory(pid_t pid, userreg_t addr, char* buf, int len);
void WriteMemory(pid_t pid, userreg_t addr, char* buf, int len = 0);
void WriteUserReg(pid_t pid, int offset, userreg_t value);
EPathArg CreateDirTree(const std::string& baseDir, char* filePath, int flags);
void ModifyPathArg(CIntercept& intercept, pid_t pid, userreg_t& address, char* path);
void ReadDirPath(pid_t pid, user_regs_struct& userRegs, char* path); 
void ReadDirPaths(pid_t pid, user_regs_struct& userRegs, char* oldPath, char* newPath);
int GetBitness(pid_t pid);

inline bool Is64Bit(const user_regs_struct& userRegs)
{
    const SUserRegs& _userRegs = (const SUserRegs&) userRegs;
    return _userRegs.m_is64Bit;
}

// This function is used:
// - in SetSyscallNumXlat().
// - when raw (untranslated) syscall number (as returned by ptrace()) is to be accessed. Eg: CSyscallState, logging functions.
// - when syscall number contained in user_regs_struct has to be portably retrieved. Eg: CSyscallHandler.
inline userreg_t& GetSyscallNumber(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return userRegs.orig_rax;
#else
    return userRegs.orig_eax;
#endif
}

// WARNING! Use this function when 64 bit equivalent of 32 bit syscall is needed. Used by CIntercept and CSyscallHandler. This is 
// because the sole basis of intercept handler invocation is SYS_XXX constants which take 64 bit personality on 64 bit systems. And 
// syscall numbers for 32 bit applications are vastly different from these SYS_XXX values.
// The second argument (it's a pointer so that callers can omit it if not needed) tells whether or not this syscall should be 
// ignored, that is to say be searched a handler for. For 64 bit apps, the value is always false for obvious reasons. However, for 32
// bit apps, we ignore it only if it doesn't exist in CSysCompat internal map (implying that it's also absent from the handler map 
// maintained by CSyscallHandler); in other words, the syscall is not tagged for interception.
inline userreg_t GetSyscallNumXlat(user_regs_struct& userRegs, bool* ignore = 0)
{
#ifdef __x86_64__
    int retval = userRegs.orig_rax;

    if ( Is64Bit(userRegs) )
    {
        ignore && (*ignore = false); 
    }
    else
    {
        bool isOk = false;
        retval = CSysCompat::GetInstance()->_Sys32To64(userRegs.orig_rax, isOk);
        ignore && (*ignore = !isOk);
    }

    return retval;
#else
    ignore && (*ignore = false); 
    return userRegs.orig_eax;
#endif
}

inline const char* GetSyscallName(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    char** syscallName[2] = {syscallName32, syscallName64};
    bool is64Bit = Is64Bit(userRegs);
    return syscallName[is64Bit][GetSyscallNumber(userRegs)];
#else
    return syscallName32[GetSyscallNumber(userRegs)];
#endif
}

inline userreg_t& GetRetStatus(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return userRegs.rax;
#else
    return userRegs.eax;
#endif
}

inline userreg_t& GetIP(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return userRegs.rip;
#else
    return userRegs.eip;
#endif
}

inline userreg_t& GetSP(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return userRegs.rsp;
#else
    return userRegs.esp;
#endif
}

inline userreg_t& GetBP(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return userRegs.rbp;
#else
    return userRegs.ebp;
#endif
}

inline userreg_t& GetArg1(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return Is64Bit(userRegs) ? userRegs.rdi : userRegs.rbx;
#else
    return userRegs.ebx;
#endif
}

inline userreg_t& GetArg2(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return Is64Bit(userRegs) ? userRegs.rsi : userRegs.rcx;
#else
    return userRegs.ecx;
#endif
}

inline userreg_t& GetArg3(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return userRegs.rdx;
#else
    return userRegs.edx;
#endif
}

// WARNING: First consider GetArg4User() before using this one!
inline userreg_t& GetArg4(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return Is64Bit(userRegs) ? userRegs.r10 : userRegs.rsi;
#else
    return userRegs.esi;
#endif
}

// x86_64 syscalls use rcx for fourth argument when in user mode; before switching over to kernel mode, the argument is copied to
// r10. Being in user mode, we use this function instead of the one above which is provided chiefly to be used with logging.
inline userreg_t& GetArg4User(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return Is64Bit(userRegs) ? userRegs.rcx : userRegs.rsi;
#else
    return userRegs.esi;
#endif
}

inline userreg_t& GetArg5(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return Is64Bit(userRegs) ? userRegs.r8 : userRegs.rdi;
#else
    return userRegs.edi;
#endif
}

inline userreg_t& GetArg6(user_regs_struct& userRegs)
{
#ifdef __x86_64__
    return Is64Bit(userRegs) ? userRegs.r9 : userRegs.rbp;
#else
    return userRegs.ebp;
#endif
}

inline userreg_t GetRetvalOffset()
{
#ifdef __x86_64__
    return RAX * WORD_SIZE;
#else
    return EAX * WORD_SIZE;
#endif
}

inline userreg_t GetSPOffset()
{
#ifdef __x86_64__
    return RSP * WORD_SIZE;
#else
    return ESP * WORD_SIZE;
#endif
}

inline userreg_t GetArg1Offset()
{
#ifdef __x86_64__
    return RDI * WORD_SIZE;
#else
    return EBX * WORD_SIZE;
#endif
}

// Used with syscall numbers to be sent to ptrace() (for instance when we change fstat() to stat()). It returns syscall number appropriate for the 
// bitness of the target process.
inline void SetSyscallNumXlat(userreg_t syscall, user_regs_struct& userRegs)
{
#ifdef __x86_64__
    GetSyscallNumber(userRegs) = Is64Bit(userRegs) ? syscall : CSysCompat::GetInstance()->_Sys64To32(syscall);
#else
    GetSyscallNumber(userRegs) = syscall;
#endif
}

inline void SetSetpgidRegs(pid_t pid, user_regs_struct& newRegs, userreg_t setpgidAddress)
{
    Is64Bit(newRegs) ? SetSetpgidRegs64(newRegs, setpgidAddress) : SetSetpgidRegs32(pid, newRegs, setpgidAddress);
}

inline void SetMmapRegs(pid_t pid, user_regs_struct& newRegs, userreg_t mmapAddress)
{
    Is64Bit(newRegs) ? SetMmapRegs64(newRegs, mmapAddress) : SetMmapRegs32(pid, newRegs, mmapAddress);
}

inline bool IsReadOnly(userreg_t flags)
{
    return ((int(flags) & 3) == O_RDONLY) && !(int(flags) & O_CREAT);
}

inline bool IsCreate(userreg_t flags)
{
    return int(flags) & O_CREAT;
}

inline bool IsSpecial(mode_t type)
{
    return ( (S_ISCHR(type) || S_ISBLK(type) || S_ISFIFO(type) || S_ISSOCK(type)) );
}

inline void BuildFullIf(const std::string& prefix, char* path)
{
    if ( *path != '/' )  
    {
        std::string cwd = prefix + '/';

        // It's safe to assume that strlen(path) after Prepend is within the system imposed PATH_MAX.
        Prepend(cwd.c_str(), path);
    }
}

inline void ReadPath(pid_t pid, user_regs_struct& userRegs, char* path)
{
    userreg_t pathAddr = GetArg1(userRegs);
    ReadMemory(pid, pathAddr, path);
}

inline void ReadPaths(pid_t pid, user_regs_struct& userRegs, char* oldPath, char* newPath)
{
    userreg_t oldAddr = GetArg1(userRegs);
    userreg_t newAddr = GetArg2(userRegs);

    ReadMemory(pid, oldAddr, oldPath);
    ReadMemory(pid, newAddr, newPath);
} 

inline bool ShouldModifyArg(EPathArg ePathArg, EBaseDir eBaseDir)
{
    return ePathArg == E_MODIF || (eBaseDir == E_NEWBAD && ePathArg == E_ORIGOK);
}
#endif
