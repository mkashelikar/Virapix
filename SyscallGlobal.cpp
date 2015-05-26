#include "Defs.h"
#include "SIParam.h"
#include "SyscallGlobal.h"
#include "FileUtil.h"
#include "ProcUtil.h"
#include "CException.h"

#include <linux/limits.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include <log4cpp/Category.hh>

// Functions local to this file.
namespace
{
EPath ChangeIfNewExist(const std::string& baseDir, char* filePath, int flags, mode_t& type);
void ChopLast(const char* filePath, char* last, int flags);
}

unsigned int GetNonNullCount(unsigned long word)
{
    unsigned int idx = 0;

    for ( ; idx < WORD_SIZE; ++idx ) 
    {
        if ( !(word & (255ul << (idx * 8))) )
        {
            break;
        }
    }

    return idx; 
}

void SetSetpgidRegs64(user_regs_struct& newRegs, userreg_t setpgidAddress)
{
    GetIP(newRegs) = setpgidAddress;
    GetArg1(newRegs) = 0ul;
    GetArg2(newRegs) = 0ul;
}

// See the comments for SetMmapRegs32() below...
void SetSetpgidRegs32(pid_t pid, user_regs_struct& newRegs, userreg_t setpgidAddress)
{
    userreg_t& sp = GetSP(newRegs);

    sp &= 0xfffffff0;
    sp -= 16;

    userreg_t args[] = { 0ul, 0ul };
    const int argCount = sizeof args/sizeof (userreg_t);

    int spOffset = 4 * (argCount - 1); 

    for ( int idx = argCount - 1; idx >= 0; spOffset -= 4 )
    {
        if ( ptrace(PTRACE_POKEDATA, pid, sp + spOffset, args[idx--] << 32) == -1 )
        {
            CException ex(errno);
            ex << '[' << pid << "] SetSetpgidRegs32() - Set stack: " << errMsg;
            throw ex;
        }
    }

    GetIP(newRegs) = setpgidAddress;
}

void SetMmapRegs64(user_regs_struct& newRegs, userreg_t mmapAddress)
{
    GetIP(newRegs) = mmapAddress;
    GetArg1(newRegs) = 0ul;
    GetArg2(newRegs) = RES_AREA_SIZE;
    GetArg3(newRegs) = PROT_READ | PROT_WRITE;

    // We are in the user mode; the fourth argument to mmap() therefore resides in rcx and not in r10.
    GetArg4User(newRegs) = MAP_PRIVATE | MAP_ANONYMOUS;
    GetArg5(newRegs) = -1;
    GetArg6(newRegs) = 0ul;
}

// Unlike x86_64 ABI in which arguments to functions are stored in registers, 32 bit ABI requires that the arguments be pushed on the
// stack. As a result, we have to set up the stack properly before invoking mmap() in the client address space.
void SetMmapRegs32(pid_t pid, user_regs_struct& newRegs, userreg_t mmapAddress)
{
    userreg_t& sp = GetSP(newRegs);

    // Align the stack pointer on 16 byte boundary as mandated by x86_64 ABI. TODO: Is this OK on a 32 bit platform?
    sp &= 0xfffffff0;

    // Adjust the stack pointer to make room for six arguments (each 4 byte) to be passed to mmap(). Remember that the stack grows 
    // downwards. We use 32 instead of 24 for two reasons:
    // 1. ptrace() on x86_64 always transfers 8 bytes even if the target application is 32 bit. If we adjust SP by only 24 bytes, we
    // risk overwriting existing stack elements. Note that in the loop below, we decrement stack by 4 bytes; this ensures that the
    // pushed elements reside correctly at consecutive addresses even though 8 bytes are transferred.
    // 2. Stack should be aligned on 16 byte boundary on x86_64. TODO: Will this work on 32 bit OS?
    sp -= 32;

    userreg_t args[] = { 0ul, RES_AREA_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, userreg_t(-1), 0ul };
    const int argCount = sizeof args/sizeof (userreg_t);

    // spOffset points to the bottom (i.e. high address) of the new stack frame. This is where the 6th argument will reside.
    // NOTE: We blatantly hard-code word size to 4 since that's what it is on IA32!
    int spOffset = 4 * (argCount - 1); 

    // Push arguments on the stack C style, i.e. from the last to the first. 32 bits left shift is required to correctly transfer
    // WORD data from 64 bit host environment to 32 bit target process. TODO: Is this because the stack grows downwards, i.e., 
    // ptrace() places the high 32 bits (from our 64 bit word) at the location we desire and discards the remaining lower bits?
    for ( int idx = argCount - 1; idx >= 0; spOffset -= 4 )
    {
        if ( ptrace(PTRACE_POKEDATA, pid, sp + spOffset, args[idx--] << 32) == -1 )
        {
            CException ex(errno);
            ex << '[' << pid << "] SetMmapRegs32() - Set stack: " << errMsg;
            throw ex;
        }
    }

    GetIP(newRegs) = mmapAddress;
}

void PrintRegs(pid_t pid, user_regs_struct& userRegs, void* pProcCtrl, bool entryExit)
{
    using namespace log4cpp;
    Category& logger = Category::getInstance(LOG_CATEGORY);
    CategoryStream ds = logger.debugStream();

    if ( !entryExit )
    {
        // On x86_64, the system call number is stored in rax. The arguments to the system call are stored in rdi, rsi, rdx, r10, r8 
        // and r9 in that order.
        // On x86, the system call number is stored in eax. The arguments to the system call are stored in ebx, ecx, edx, esi, edi and
        // ebp in that order.
        ds << '[' << pid << "] [" << (Is64Bit(userRegs) ? "64" : "32") << " bit] Enter " 
            << GetSyscallName(userRegs) << "() AX " << (int) GetRetStatus(userRegs) << " arg1: " << (int) GetArg1(userRegs) 
            << " arg2: " << (int) GetArg2(userRegs) << " arg3: " << (int) GetArg3(userRegs) << " arg4: " << (int) GetArg4(userRegs) 
            << " arg5: " << (int) GetArg5(userRegs) << " arg6: " << (int) GetArg6(userRegs) << eol;
        ds.flush();
    }
    else
    {
        ds << '[' << pid << "] Exit " << GetSyscallName(userRegs) << "() retval " << GetRetStatus(userRegs) << eol;
        ds.flush();
    }
}

void ReadMemory(pid_t pid, userreg_t addr, char* buf)
{
    UMemContent data;
    int nonNullCount = 0; 
    int size = 0;
    int incr = 0;

    do
    {
        // -1 can be a non-error return value from PTRACE_PEEKDATA. We need to distinguish the two.
        errno = 0;

        if ( (data.m_val = ptrace(PTRACE_PEEKDATA, pid, addr + incr, 0)) == -1 )
        {
            if ( errno )
            {
                CException ex(errno);
                ex << '[' << pid << "] ReadMemory() - " << errMsg;
                throw ex;
            }
        }

        if ( nonNullCount = GetNonNullCount(data.m_val) ) // sic!
        {
            memcpy(buf + incr, data.m_strVal, nonNullCount);
        }

        incr += WORD_SIZE;
        size += nonNullCount;
    } while ( nonNullCount == WORD_SIZE && size < PATH_MAX ); // We use system limit PATH_MAX. It's left to our caller to truncate
                                                              // it if required.

    buf[size] = 0;
}

void ReadMemory(pid_t pid, userreg_t addr, char* buf, int len)
{
    UMemContent data;

    int tail = len % WORD_SIZE;
    int incr = 0;

    for ( int iter = len / WORD_SIZE; iter; --iter )
    {
        // -1 can be a non-error return value from PTRACE_PEEKDATA. We need to distinguish the two.
        errno = 0;

        if ( (data.m_val = ptrace(PTRACE_PEEKDATA, pid, addr + incr, 0)) == -1 )
        {
            if ( errno )
            {
                CException ex(errno);
                ex << '[' << pid << "] ReadMemory() - " << errMsg;
                throw ex;
            }
        }

        memcpy(buf + incr, data.m_strVal, WORD_SIZE);
        incr += WORD_SIZE;
    }

    if ( tail )
    {
        errno = 0;

        if ( (data.m_val = ptrace(PTRACE_PEEKDATA, pid, addr + incr, 0)) == -1 )
        {
            if ( errno )
            {
                CException ex(errno);
                ex << '[' << pid << "] ReadMemory() - " << errMsg;
                throw ex;
            }
        }

        memcpy(buf + incr, data.m_strVal, tail);
    }
}

void WriteMemory(pid_t pid, userreg_t addr, char* buf, int len)
{
    UMemContent data;

    int size = len ? len : (strlen(buf) + 1); // 1 for the null terminator
    int tail = size % WORD_SIZE;
    int incr = 0;

    // First put WORD_SIZE X bytes into the client address space...
    for ( int iter = size / WORD_SIZE; iter; --iter )
    {
        memcpy(data.m_strVal, buf + incr, WORD_SIZE);

        if ( ptrace(PTRACE_POKEDATA, pid, addr + incr, data.m_val) == -1 )
        {
            CException ex(errno);
            ex << '[' << pid << "] WriteMemory() - " << errMsg;
            throw ex;
        }

        incr += WORD_SIZE;
    }

    // ...now put the remainder, if any.
    if ( tail )
    {
        memcpy(data.m_strVal, buf + incr, tail);

        if ( ptrace(PTRACE_POKEDATA, pid, addr + incr, data.m_val) == -1 )
        {
            CException ex(errno);
            ex << '[' << pid << "] WriteMemory() - " << errMsg;
            throw ex;
        }
    }
}

void WriteUserReg(pid_t pid, int offset, userreg_t value)
{
    if ( ptrace(PTRACE_POKEUSER, pid, offset, value) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] WriteUserReg() - " << errMsg;
        throw ex;
    }
}

// The functions in the anonymous namespace are private to this file.
namespace
{

// This function expects a normalised path, i.e. path without ".", ".." or a trailing '/'. More importantly, the path should
// ***not*** be prefixed with the new root dir. 
EPath ChangeIfNewExist(const std::string& baseDir, char* filePath, int flags, mode_t& type)
{
    using namespace std;
    using namespace log4cpp;

    EPath retval = E_BAD;
    string newHierarchy;
    struct stat fileInfo;

    // Prepend the new root dir only if the new hierarchy is not already deduced as bad
    if ( !(flags & F_BADNEWDIR) )
    {
        newHierarchy = baseDir + filePath;
    } 

    // Some explanation is in order. If we reach here, it means the hierarchy contained in filePath does not exist on the
    // virtual machine. Now if filePath contains the user home area on the host system ***and*** this isn't a file creation
    // system call, we make it refer to the directory (even if non-existent) on virtual machine, not to the one on the host
    // system. For one, the home directory stores application specific configuration information which can differ across
    // machines. And we don't want to refer to a wrong file. In such a case, the application typically creates the missing
    // file in a subsequent syscall. On the other hand, if this is a file creation syscall (irrespective of whether the flag
    // F_OLDHOMEDIR is set or not), we proceed as usual and replicate the hierarchy (up to the to-be-created file) on the 
    // virtual machine if required.
    if ( (flags & F_OLDHOMEDIR) && !(flags & F_CREATE) )
    {
        strcpy(filePath, newHierarchy.c_str());
        return E_OLDHOMEDIR;
    }

    type = 0;

    // lstat() for dummy symbolic links that point to non-existent files (Firefox creates one such link for the lock file). stat() 
    // returns -1 for such symbolic link whereas lstat() returns 0 which is what we want. Note that fileInfo obtained here is not
    // used anywhere even though its st_mode field is saved in type. The check for F_BADNEWDIR spares us the trouble of calling 
    // lstat() since the invalidity is already established.
    if ( (flags & F_BADNEWDIR) || lstat(newHierarchy.c_str(), &fileInfo) ) // The new path doesn't exist...
    {
        // Don't replicate the original directory structure if new doesn't exist. Used typically for read-only syscalls or 
        // syscalls that remove files or read symbolic links. It makes little sense to construct the directory hierarchy in 
        // these situations.
        if ( flags & F_NOREPLIC )
        {
            retval = ( access(filePath, F_OK) ? E_BAD : E_NOCREATE ); // Set the correct return value!

            // Since we don't want to change the host system, we set filePath to a null string. That way, the system call will
            // fail and prevent any changes. The flag F_DELETE indicates that the system call deletes the file. 
            // TODO: Is this a good strategy?
            if ( flags & F_DELETE ) 
            {
                *filePath = 0;
                retval = E_OLDNODEL;
            }
        }
        else
        {
            if ( stat(filePath, &fileInfo) ) // Old path doesn't exist either...
            { 
                if ( errno != ENOENT ) // Obviously a malformed path...
                {
                    return E_BAD;
                }

                // That's why the requirement that there shouldn't be any trailing slash! By terminating filePath at the location ptr,
                // filePath now holds only the parent directory.
                char* ptr = strrchr(filePath, '/');
                *ptr = 0; 

                // Quite a clutter, but let's take on each of the conditions one by one...
                // Sub-condition 1.1 - Parent directory in filePath is not a root directory; remember the expression *ptr = 0.
                // Sub-condition 1.2 - Parent directory in filePath does not exist.
                // The above two sub-conditions together mean that the ***original*** path is invalid; after all, there's no system 
                // call that allows you to create two path components in a single invocation. On the other hand, it's likely that the
                // path may exist in the new hierarchy. This typically happens after a successful mkdir() call when the new directory
                // is created in the new hierarchy (after our interception) and not at the original location. This is what we 
                // validate in the first if construct.
                // Condition 2 - Means that this system call does not create any new file or directory. This condition is evaluated 
                // if 1.1 and 1.2 fail, i.e. the parent directory is valid, but the final component is not and the system call is not
                // configured to create it. An obvious error.
                if ( (strrchr(filePath, '/') && access(filePath, F_OK)) ) // Sub-conditions 1.1 & 1.2
                {
                    int pos = newHierarchy.rfind('/');
                    newHierarchy[pos]= 0;
                    int status = access(newHierarchy.c_str(), F_OK);
                    newHierarchy[pos]= '/'; // IMP!
                    retval = status ? E_BAD : E_PARENTEXIST;
                }
                else 
                {
                    // Condition 2
                    retval = (flags & F_CREATE) ? E_PARENTEXIST : E_BAD;
                }

                *ptr = '/'; // IMP!
            }
            else
            {
                type = fileInfo.st_mode;

                // We reach here if the new path doesn't exist, but the old does. If the old path refers to special file and the
                // system call is not the one that creates a new file, there's nothing to do. After all, we just can't replicate this
                // sort of file through normal copy semantics. And there's no point in creating new directory hierarchy either. TODO:
                // This means that the system call will continue with original values resulting in modification of the file on the
                // host system. Any better ideas?
                retval = ( !(flags & F_CREATE) && IsSpecial(type) ) ? E_NOCREATE : E_OLDEXIST;
            }
        }
    }
    else
    {
        type = fileInfo.st_mode;
        strcpy(filePath, newHierarchy.c_str());
        retval = E_NEWEXIST;
    }

    return retval;
}

// This function assumes that the path has been validated by ChangeIfNewExist() above.
void ChopLast(const char* filePath, char* last, int flags)
{
    using namespace log4cpp;
    Category& logger = Category::getInstance(LOG_CATEGORY);
    CategoryStream ds = logger.debugStream();

    struct stat fileInfo;
    int status = stat(filePath, &fileInfo);

    if ( status ) // ENOENT since other cases are filtered out by ChangeIfNewExist()
    {
        ds << "ChopLast() - ENOENT." << eol;
        ds.flush();

        // Since ChangeIfNewExist() has certified the path as valid, the only reason for ENOENT is file or directory creation. 
        // Since that task will be performed by the system call itself, we are only responsible for replicating the path till 
        // the parent. As such, we temporarily chop the last component.
        *last = 0;
    }
    else
    {
        // filePath exists. Temporarily eliminate the last component if it isn't a directory.
        if ( !S_ISDIR(fileInfo.st_mode) )
        {
            ds << "ChopLast() - File exists: non-directory." << eol;
            ds.flush();
            *last = 0;
        }
        else
        {
            ds << "ChopLast() - File exists: directory." << eol;
            ds.flush();

            // Looks strange: F_CREATE flag when file exists! But there's every likelihood of an application issuing a directory
            // creation system call when the directory is very much present.
            if ( flags & F_CREATE )
            {
                *last = 0;
            }
        }
    }
}

}

// IMP: filePath must be an absolute path (beginning with a '/'). The function returns true if filePath is modified, i.e. the syscall
// argument needs updating.
EPathArg CreateDirTree(const std::string& baseDir, char* filePath, int flags)
{
    using namespace std;
    using namespace log4cpp;

    Category& logger = Category::getInstance(LOG_CATEGORY);
    CategoryStream ds = logger.debugStream();
    mode_t fileType = 0;
    EPath status = ChangeIfNewExist(baseDir, filePath, flags, fileType);

    // If the original path passed to the system call itself is bad, we simply forward it without modification. After all, it's an
    // application error and it's not our task to fix it!
    if ( status != E_OLDEXIST && status != E_PARENTEXIST )
    {
        ds << "CreateDirTree() - " << (status == E_NEWEXIST ? "Path exists in the new hierarchy." : (status == E_BAD ? "Invalid "
                    "path." : (status == E_NOCREATE ? "Using original path on host system." : (status == E_OLDHOMEDIR ? "Path lies in "
                    "old home dir." : "Ignoring delete for file on host system." )))) << eol;
        ds.flush();
        return ( ((status == E_NEWEXIST) || (status == E_OLDHOMEDIR) || (status == E_OLDNODEL)) ? E_MODIF : 
                (status == E_NOCREATE ? E_ORIGOK : E_ORIGBAD) );
    }

    char* ptr1 = 0;
    char* ptr2 = filePath;
    char* last = strrchr(filePath, '/');

    ChopLast(filePath, last, flags); 

    string newHierarchy = baseDir;

    // The shorthand notation used in the loop is to reduce the size of an otherwise tediously long function. Admittedly there are
    // some extra checks out there, but these help avoid coding another loop.
    for ( bool createDir = false; ptr2; ) // sic!
    {
        ( ptr1 = strchr(ptr2 + 1, '/') ) && ( *ptr1 = 0 ); // sic!
        newHierarchy += ptr2;
        ptr1 && (*ptr1 = '/'); // Restore the string to its original length.
        ptr2 = ptr1;

        // Setting createDir eliminates the call to access() once we find a directory that doesn't exist. 
        if ( createDir || access(newHierarchy.c_str(), F_OK) )
        {
            createDir = true;
            ds << "CreateDirTree() - Creating directory " << newHierarchy << eol; 
            ds.flush();
            mkdir(newHierarchy.c_str(), S_IRWXU);
        }
    }

    if ( (flags & F_CREATE) || !S_ISDIR(fileType) )
    {
        // Important! Restore the original string.
        *last = '/';
        newHierarchy += last; // The file name which we knowingly ignored till now ;-)
    }

    // Update the argument to be passed to the system call.
    strcpy(filePath, newHierarchy.c_str());
    return E_MODIF;
}

void ReadDirPath(pid_t pid, user_regs_struct& userRegs, char* path)
{
    userreg_t dirFd = GetArg1(userRegs);
    userreg_t pathAddr = GetArg2(userRegs);

    // Non-standard extension in Linux. If pathAddr is NULL, dirFd refers to absolute path.
    if ( !pathAddr )
    {
        FdToPath(pid, dirFd, path); // Note the third argument.
        return;
    }

    ReadMemory(pid, pathAddr, path);

    // The typecast to int is essential while dealing with 32 bit applications. The value of AT_FWD (-100) which is 4 bytes there
    // doesn't translate correctly when copied to 8 byte userreg_t in x86_64 land.
    if ( *path != '/' && int(dirFd) != AT_FDCWD ) // The path is relative and dirFd references a valid directory.
    {
        char dirPath[PATH_MAX];
        FdToPath(pid, dirFd, dirPath);

        int dirPathLen = strlen(dirPath);
        dirPath[dirPathLen] = '/';
        dirPath[dirPathLen + 1] = 0;
        Prepend(dirPath, path);
    }
}

void ReadDirPaths(pid_t pid, user_regs_struct& userRegs, char* oldPath, char* newPath)
{
    int syscall64 = GetSyscallNumXlat(userRegs);

    userreg_t oldDirFd = (syscall64 != SYS_symlinkat) ? GetArg1(userRegs) : 0;
    userreg_t oldAddr = (syscall64!= SYS_symlinkat) ? GetArg2(userRegs) : GetArg1(userRegs);
    userreg_t newDirFd = (syscall64 != SYS_symlinkat) ? GetArg3(userRegs) : GetArg2(userRegs);
    userreg_t newAddr = (syscall64 != SYS_symlinkat) ? GetArg4(userRegs) : GetArg3(userRegs);

    char dirPath[PATH_MAX] = {0};
    int dirPathLen = 0;

    ReadMemory(pid, oldAddr, oldPath);
    ReadMemory(pid, newAddr, newPath);

    if ( syscall64 != SYS_symlinkat )
    {
        if ( *oldPath != '/' && oldDirFd != AT_FDCWD ) // The path is relative and oldDirFd references a valid directory.
        {
            FdToPath(pid, oldDirFd, dirPath);

            dirPathLen = strlen(dirPath);
            dirPath[dirPathLen] = '/';
            dirPath[dirPathLen + 1] = 0;
            
            Prepend(dirPath, oldPath);
        }
    }

    if ( *newPath != '/' && newDirFd != AT_FDCWD ) // The path is relative and newDirFd references a valid directory.
    {
        FdToPath(pid, newDirFd, dirPath);

        dirPathLen = strlen(dirPath);
        dirPath[dirPathLen] = '/';
        dirPath[dirPathLen + 1] = 0;

        Prepend(dirPath, newPath);
    }
}

int GetBitness(pid_t pid)
{
#ifdef __x86_64__
    userreg_t mode = ptrace(PTRACE_PEEKUSER, pid, CS * WORD_SIZE, 0);
    assert(mode == MODE_64 || mode == MODE_32);
    return mode == MODE_64 ? 64 : 32;
#else
    return 32;
#endif
}
