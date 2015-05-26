#include "FileUtil.h"
#include "ProcUtil.h"
#include "SIParam.h"
#include "SyscallGlobal.h"
#include "CSyscallHandler.h"
#include "CIntercept.h"
#include "CException.h"

#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <linux/limits.h>
#include <linux/un.h>
#include <linux/net.h>
#include <sched.h>
#include <stdint.h>

#include <log4cpp/Category.hh>

////////////////////////////////////////////// Inline methods /////////////////////////////////////////////////////////////

inline bool CSyscallHandler::_IsShlib(const char* path)
{
    return !regexec(&m_shlibRegex, basename(path), 0, 0, 0);
}

inline void CSyscallHandler::_GetPageRgn(CIntercept& intercept, pid_t pid, userreg_t& pathAddr)
{
    using namespace log4cpp;

    // pathAddr refers to one of the registers currently used by the system call and which holds the address of the buffer containing
    // the file path. Our task is to point it to "our" buffer which would contain the modified path. This buffer had long been 
    // allocated by inserting mmap() invocation in the client process. An appropriate slot in that page is what is returned by 
    // intercept._GetPageRgn(). Note that pathAddr is passed by reference since it's part of the larger user_regs_struct which we'll 
    // later pass on to ptrace().
    pathAddr = intercept._GetPageRgn(pid);

    if ( pathAddr == userreg_t (-1) )
    {
        CException ex;
        ex << '[' << pid << "] CSyscallHandler::_GetPageRgn() - No more page slots to hold path!";
        throw ex;
    }
}

inline bool IgnoreSockcall(userreg_t code)
{
    return code != SYS_BIND && code != SYS_CONNECT && code != SYS_SENDTO && code != SYS_SENDMSG;
}

// This silly function is used solely for logging.
inline const char* GetSockcallProc(userreg_t code)
{
    return code == SYS_BIND ? "sys_bind()" : (code == SYS_CONNECT ? "sys_connect()" : (code == SYS_SENDTO ? "sys_sendto()" :
               (code == SYS_SENDMSG ? "sys_sendmsg()" : "sys_invalid"))); 
}

////////////////////////////////////////////// Syscall handlers  /////////////////////////////////////////////////////////////

void CSyscallHandler::_OnOpen(SIParam& param)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnOpen() - Into " << GetSyscallName(param.m_userRegs) 
            << "()" << eol;
        m_logger.debugStream().flush(); 

        char path[PATH_MAX] = {0};
        ReadPath(param.m_pid, param.m_userRegs, path);
        _DoOnOpen(param, path);
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnOpen() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 

        // _ReleasePageRgn() needs to be called only if _ModifyPathArg() is also called. However, since we don't maintain any state
        // (remember that CSyscallHandler is a singleton shared by multiple threads and keeping track of path modifications for
        // every system call per thread introduces unnecessary complexity not to mention synchronisation issues), we call 
        // _ReleasePageRgn() unconditionally. And anyway, it's not an error to call the function this way: the intercept object takes
        // care of this.
        param.m_intercept._ReleasePageRgn(param.m_pid);
    }
}

void CSyscallHandler::_OnStat(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnFstat(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon2(param, SYS_stat);
}

void CSyscallHandler::_OnLstat(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnAccess(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnConnect(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSockCommon(param);
}

void CSyscallHandler::_OnSendto(SIParam& param)
{
    param.m_flags |= (F_NOREPLIC | F_SOCKSENDTO);
    _OnSockCommon(param);
}

void CSyscallHandler::_OnSendmsg(SIParam& param)
{
    param.m_flags |= (F_NOREPLIC | F_SOCKSENDMSG);
    _OnSockCommon(param);
}

void CSyscallHandler::_OnBind(SIParam& param)
{
    param.m_flags |= F_CREATE;
    _OnSockCommon(param);
}

void CSyscallHandler::_OnClone(SIParam& param)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnClone() - Entering..." << eol;
        m_logger.debugStream().flush(); 
        
        userreg_t cloneFlags = GetArg1(param.m_userRegs);

        if ( cloneFlags & CLONE_UNTRACED )
        {
            m_logger.warnStream() << '[' << param.m_pid << "] CSyscallHandler::_OnClone() - Unsetting CLONE_UNTRACED flag." << eol;
            m_logger.warnStream().flush(); 

            cloneFlags &= ~CLONE_UNTRACED;
            WriteUserReg(param.m_pid, GetArg1Offset(), cloneFlags);
        }
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnClone() - Returning. Retval: " << 
            int (GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 
    }
}

void CSyscallHandler::_OnExecve(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _DoOnExecve(param);
}

// Used by 32 bit applications: need to treat this case specially!
void CSyscallHandler::_OnSocketcall(SIParam& param)
{
    using namespace log4cpp;

    // Identify the syscall.
    userreg_t code = GetArg1(param.m_userRegs);
    
    if ( IgnoreSockcall(code) )
    {
        return;
    }

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSocketCall() - Into " << GetSockcallProc(code) << eol;
        m_logger.debugStream().flush(); 

        UMemContent data;
        data.m_val = 0;

        // The second argument to socketcall() contains the address of the first element of the array of type unsigned long. The 
        // array represents the arguments to be passed to the underlying socket syscall.
        userreg_t pArg = GetArg2(param.m_userRegs);

        // We take the liberty to assume pointer size as 4 on 32 bit applications. pArg is of non-pointer type and as such the
        // mathematics involves explicit multiplier 4. sockaddr_in appears at position 5 (offset 4 or 16 bytes) for sys_sendto() and 
        // at position 2 (offset 1 or 4 bytes) for other syscalls of interest to us. sockaddrPtr32 is a double pointer.
        userreg_t sockaddrPtr32 = pArg + ((code == SYS_SENDTO) ? 16 : 4);

        // The following call deferences sockaddrPtr32, i.e. fetches the address of sockaddr_in object.
        ReadMemory(param.m_pid, sockaddrPtr32, data.m_strVal, WORD_SIZE);

        // We fetched a word (8 bytes), but 32 bit apps only use 4 byte pointers. Clear the upper 4 bytes.
        data.m_val &= 0xffffffff;
        param.m_flags |= F_SOCKCALL32;

        (code == SYS_BIND) && (param.m_flags |= F_CREATE);
        (code != SYS_BIND) && (param.m_flags |= F_NOREPLIC);

        // The position of sockaddr_in argument is different in sys_sendto() than in other syscalls.
        (code == SYS_SENDTO) && (param.m_flags |= F_SOCKSENDTO);

        // data.m_val now contains the address of sockaddr_in structure.
        _OnSockCommon(param, data.m_val);
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSocketCall() - Returning from " 
            << GetSockcallProc(code) << ". Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 
    }
}

void CSyscallHandler::_OnTruncate(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnChdir(SIParam& param)
{
    using namespace log4cpp;
    
    char path[PATH_MAX];

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnChdir() - Into " << GetSyscallName(param.m_userRegs) 
                << "()" << eol;
        m_logger.debugStream().flush(); 

        assert(param.m_intercept.m_cwd[0] == '/');
        ReadPath(param.m_pid, param.m_userRegs, path);
    }

    _DoOnChdir(param, path, 0);
}

void CSyscallHandler::_OnFchdir(SIParam& param)
{
    using namespace log4cpp;
    
    char path[PATH_MAX];
    bool status = true;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnFchdir() - Into " << GetSyscallName(param.m_userRegs) 
            << "()" << eol;
        m_logger.debugStream().flush(); 

        assert(param.m_intercept.m_cwd[0] == '/');

        userreg_t& arg1 = GetArg1(param.m_userRegs); // The function returns a reference

        try
        {
            status = FdToPath(param.m_pid, arg1, path);
        }

        catch ( CException& ex )
        {
            ex << '[' << param.m_pid << "] CSyscallHandler::_OnFchdir() - Error retrieving path from descriptor. " << ex.StrError();
            throw ex;
        }
    }

    if ( status ) // Means either of F_EXIT or (F_ENTRY and a successful call to FdToPath())
    {
        _DoOnChdir(param, path, SYS_chdir);
    }
    else
    {
        if ( param.m_flags & F_ENTRY ) // Log correct information!
        {
            m_logger.warnStream() << '[' << param.m_pid << "] CSyscallHandler::_OnFchdir() - Descriptor refers to deleted dir." 
                << eol; 
            m_logger.warnStream().flush(); 
        }
    }
}

void CSyscallHandler::_OnRename(SIParam& param)
{
    _OnSyscallCommon3(param);
}

void CSyscallHandler::_OnMkdir(SIParam& param)
{
    param.m_flags |= F_CREATE;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnRmdir(SIParam& param)
{
    param.m_flags |= (F_DELETE | F_NOREPLIC);
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnCreat(SIParam& param)
{
    param.m_flags |= F_CREATE;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnLink(SIParam& param)
{
    // No F_NOREPLIC flag here because hard links don't work across mount points.
    _OnSyscallCommon3(param);
}

void CSyscallHandler::_OnUnlink(SIParam& param)
{
    param.m_flags |= (F_DELETE | F_NOREPLIC);
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnSymlink(SIParam& param)
{
    param.m_flags |= F_NOREPLIC; // Because symbolic links are legal across mount points
    _OnSyscallCommon3(param);
}

void CSyscallHandler::_OnReadlink(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnChmod(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnFchmod(SIParam& param)
{
    _OnSyscallCommon2(param, SYS_chmod);
}

void CSyscallHandler::_OnChown(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnFchown(SIParam& param)
{
    _OnSyscallCommon2(param, SYS_chown);
}

void CSyscallHandler::_OnLchown(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnUtime(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnMknod(SIParam& param)
{
    param.m_flags |= F_CREATE;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnAcct(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnSetxattr(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnLsetxattr(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnFsetxattr(SIParam& param)
{
    _OnSyscallCommon2(param, SYS_setxattr);
}

void CSyscallHandler::_OnGetxattr(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnLgetxattr(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnFgetxattr(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon2(param, SYS_getxattr);
}

void CSyscallHandler::_OnListxattr(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnLlistxattr(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnFlistxattr(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallCommon2(param, SYS_listxattr);
}

void CSyscallHandler::_OnRemovexattr(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnLremovexattr(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnFremovexattr(SIParam& param)
{
    _OnSyscallCommon2(param, SYS_removexattr);
}

void CSyscallHandler::_OnUtimes(SIParam& param)
{
    _OnSyscallCommon(param);
}

void CSyscallHandler::_OnOpenat(SIParam& param)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnOpenat() - Into " << GetSyscallName(param.m_userRegs) 
            << "()" << eol;
        m_logger.debugStream().flush(); 

        char path[PATH_MAX] = {0};
        ReadDirPath(param.m_pid, param.m_userRegs, path);
        _DoOnOpen(param, path);
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnOpenat() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 

        param.m_intercept._ReleasePageRgn(param.m_pid);
    }
}

void CSyscallHandler::_OnMkdirat(SIParam& param)
{
    param.m_flags |= F_CREATE;
    _OnSyscallAtCommon(param);
}

void CSyscallHandler::_OnMknodat(SIParam& param)
{
    param.m_flags |= F_CREATE;
    _OnSyscallAtCommon(param);
}

// The system call fchownat() should have been named chownat() since it takes path as an argument, not a file descriptor.
void CSyscallHandler::_OnFchownat(SIParam& param)
{
    _OnSyscallAtCommon(param);
}

// The system call futimesat() should have been named utimesat() since it takes path as an argument, not a file descriptor.
void CSyscallHandler::_OnFutimesat(SIParam& param)
{
    _OnSyscallAtCommon(param);
}

// The system call fstatat() should have been named statat() since it takes path as an argument, not a file descriptor.
void CSyscallHandler::_OnFstatat(SIParam& param)
{
    _OnSyscallAtCommon(param);
}

void CSyscallHandler::_OnUnlinkat(SIParam& param)
{
    param.m_flags |= (F_NOREPLIC | F_DELETE);
    _OnSyscallAtCommon(param);
}

void CSyscallHandler::_OnRenameat(SIParam& param)
{
    _OnSyscallAtCommon3(param);
}

void CSyscallHandler::_OnLinkat(SIParam& param)
{
    // No F_NOREPLIC flag here because hard links don't work across mount points.
    _OnSyscallAtCommon3(param);
}

void CSyscallHandler::_OnSymlinkat(SIParam& param)
{
    param.m_flags |= F_NOREPLIC; // Because symbolic links are legal across mount points
    _OnSyscallAtCommon3(param);
}

void CSyscallHandler::_OnReadlinkat(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallAtCommon(param);
}

// The system call fchmodat() should have been named chmodat() since it takes path as an argument, not a file descriptor.
void CSyscallHandler::_OnFchmodat(SIParam& param)
{
    _OnSyscallAtCommon(param);
}

// The system call faccessat() should have been named accessat() since it takes path as an argument, not a file descriptor.
void CSyscallHandler::_OnFaccessat(SIParam& param)
{
    param.m_flags |= F_NOREPLIC;
    _OnSyscallAtCommon(param);
}

void CSyscallHandler::_OnUtimensat(SIParam& param)
{
    _OnSyscallAtCommon(param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSyscallHandler::_DoOnOpen(SIParam& param, char* path)
{
    using namespace log4cpp;

    EBaseDir rStatus = _RebuildPath(param.m_pid, param.m_intercept.m_cwd, path);

    if ( rStatus == E_NEWOK )
    {
        return;
    }

    if ( rStatus == E_OLDHOME )
    {
        param.m_flags |= F_OLDHOMEDIR;
    }
    else if ( rStatus == E_NEWBAD )
    {
        param.m_flags |= F_BADNEWDIR;
    }

    struct stat fileInfo;
    mode_t type = (stat(path, &fileInfo) ? 0 : fileInfo.st_mode);
    
    if ( !IsSpecial(type) ) // We aren't interested in special files like char/block/socket/FIFO
    {
        int syscallNum = GetSyscallNumXlat(param.m_userRegs);
        userreg_t& pathAddr = (syscallNum == SYS_open) ? GetArg1(param.m_userRegs) : GetArg2(param.m_userRegs);
        int openFlags = (syscallNum == SYS_open) ? GetArg2(param.m_userRegs) : GetArg3(param.m_userRegs);
        EPathArg pStatus;

        if ( IsReadOnly(openFlags) )
        {
            m_logger.infoStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnOpen() - File opened read-only." << eol;
            m_logger.infoStream().flush(); 

            // It may be more efficient to cache the modified path (after a write) rather than build it all over again for read. 
            // However, the space factor rules out this option. 

            param.m_flags |= F_NOREPLIC;
            pStatus = _ModifyPath(param.m_pid, path, param.m_flags); 

            if ( ShouldModifyArg(pStatus, rStatus) )
            {
                // Get a free page slot and put its address in the first argument register. Note that pathAddr (itself a
                // reference to one of the registers in userRegs) is passed by reference.
                _GetPageRgn(param.m_intercept, param.m_pid, pathAddr);
                
                //Finally, write back the modified register into the client memory.
                _ModifyPathArg(param.m_pid, param.m_userRegs, pathAddr, path);
            }
        }
        else
        {
            m_logger.infoStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnOpen() - " << (IsCreate(openFlags) ? "File "
                    "creation." : "File opened for writing.") << eol;
            m_logger.infoStream().flush(); 

            param.m_flags |= (IsCreate(openFlags) ? F_CREATE : 0);

            // Modify path so that it appears under the new hierarchy. Optionally truncate the name to zero if it doesn't fit 
            // within the PATH_MAX_LEN imposed by us.
            pStatus = _ModifyPath(param.m_pid, path, param.m_flags);

            if ( ShouldModifyArg(pStatus, rStatus) )
            {
                _GetPageRgn(param.m_intercept, param.m_pid, pathAddr);
                _ModifyPathArg(param.m_pid, param.m_userRegs, pathAddr, path);
            }
        }
    }
}

void CSyscallHandler::_DoOnChdir(SIParam& param, char* path, int altSyscall )
{
    using namespace log4cpp;
    using namespace std;
    
    string& cwd = param.m_intercept.m_cwd;
    string& cwdOnHold = param.m_intercept.m_cwdOnHold;

    if ( param.m_flags & F_ENTRY )
    {
        // The semantics of _RebuildDirPath() used with chdir() are slightly different from _RebuildPath() used with other syscalls.
        // Whereas the latter returns false if the directory path passed to the syscall is already rooted at the new hierarchy, the
        // former has no return value. For chdir(), we choose the directory on the host system over that located in the new hierarchy;
        // for other syscalls (except open() for a directory which has the same semantics as for chdir()) , it's exactly the opposite.
        // We first construct an absolute path (we do this with other syscalls too) and if the path exists on the host system, we 
        // change to that location, else change to the corresponding location in the new hierarchy. After all, an application is more
        // likely to find files in the old location than in the new one and consequently using the old directory as cwd ensures that 
        // the application doesn't fail if it uses paths relative to that directory.
        _RebuildDirPath(param.m_pid, cwd, path);

        // The last argument tells whether or not the path was stripped prior to calling _ModifyDirPath()
        if ( _ModifyDirPath(param.m_pid, path) )
        {
            userreg_t& arg1 = GetArg1(param.m_userRegs);

            // _OnFchdir() sets altSyscall to SYS_chdir. See the comment in _DoOnSyscallCommon2() for explanation.
            if ( altSyscall )
            {
                SetSyscallNumXlat(altSyscall, param.m_userRegs);

                // Notify CSyscallState that we changed syscall. Note: SetSyscallNumXlat() above translates altSyscall for 32 bit processes (if 
                // running on x86_64 platform) and stores it in param.m_userRegs. It's this translated number that we send to ptrace() (through 
                // _ModifyPathArg()) and that we retrieve through a call to GetSyscallNumber(). The call is needed because CSyscallState records the
                // original (i.e. as received from ptrace()) syscall number and altSyscall contains x86_64 syscall number. For native processes, the
                // translation is expectedly skipped.
                param.m_intercept._Notify(NOTIFY_ALTSYSCALL, param.m_pid, GetSyscallNumber(param.m_userRegs));
            }

            _GetPageRgn(param.m_intercept, param.m_pid, arg1);
            _ModifyPathArg(param.m_pid, param.m_userRegs, arg1, path);
        }

        // We'll have to wait till chdir() returns. The register storing arg1 changes between syscall entry and exit and so we have 
        // to keep a copy around.
        cwdOnHold = path;
    }
    else
    {
        if ( !GetRetStatus(param.m_userRegs) ) // chdir() was successful. Update intercept.m_cwd
        {
            cwd = cwdOnHold;
        }
        else
        {
            m_logger.warnStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnChdir() - " << cwdOnHold << " is an invalid path!"
                << eol;
            m_logger.warnStream().flush();
        }

        cwdOnHold = "";

        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnChdir() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 

        param.m_intercept._ReleasePageRgn(param.m_pid);
    }
}

void CSyscallHandler::_DoOnSendmsg(SIParam& param, char* data)
{
    using namespace log4cpp;
    msghdr* pMsgHdr = reinterpret_cast<msghdr*> (data); // Pointer to msghdr in ***our address space***
    sockaddr_un sockAddrUn;

    // NOTE: We don't use reference to msgHdrAddr (compare with other syscall handlers) because like other socket syscalls, we don't 
    // modify this parameter for sendmsg() either.
    userreg_t msgHdrAddr = GetArg2(param.m_userRegs); // The address of msghdr structure ***in client address space***
    char* sockAddrPtr = (char*) pMsgHdr->msg_name;
    socklen_t sockAddrSize = pMsgHdr->msg_namelen;

    if ( !sockAddrPtr || !sockAddrSize )
    {
        return;
    }

    // Get sockaddr_un structure from the address above. See the concern raised in _OnSockCommon().
    ReadMemory(param.m_pid, userreg_t(sockAddrPtr), (char*) &sockAddrUn, sizeof (sockaddr_un));

    char* sockName = sockAddrUn.sun_path;

    if ( sockAddrUn.sun_family == AF_UNIX ) // We are interested only in UNIX domain sockets
    {
        sockName[UNIX_PATH_MAX - 1] = 0; // Just in case
        
        // Abstract sockets have null as the first byte of sun_path and don't appear in the file system.
        if ( !sockName[0] )
        {
            m_logger.infoStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnSendmsg() - Ignoring abstract socket/connection "
                << &sockName[1] << eol;
            m_logger.infoStream().flush(); 
            return;
        }

        EBaseDir rStatus = _RebuildPath(param.m_pid, param.m_intercept.m_cwd, sockName);

        if ( rStatus == E_NEWOK )
        {
            return;
        }

        if ( rStatus == E_OLDHOME )
        {
            param.m_flags |= F_OLDHOMEDIR;
        }
        else if ( rStatus == E_NEWBAD )
        {
            param.m_flags |= F_BADNEWDIR;
        }

        EPathArg pStatus = _ModifyPath(param.m_pid, sockName, param.m_flags, UNIX_PATH_MAX);

        if ( ShouldModifyArg(pStatus, rStatus) )
        {
            sockAddrSize = sizeof sockAddrUn.sun_family + strlen(sockName) + 1; // 1 for the null terminator.

            // We don't change any of the sycall arguments; instead (as with socketcall()), we change the data pointed to by them.
            WriteMemory(param.m_pid, userreg_t(sockAddrPtr), (char*) &sockAddrUn, sockAddrSize);
            WriteMemory(param.m_pid, msgHdrAddr, (char*) pMsgHdr, sizeof (msghdr));
        }
    }
}

void CSyscallHandler::_DoOnExecve(SIParam& param)
{
    using namespace log4cpp;

    // Nothing for syscall exit because execve() doesn't return (even though occasionally ptrace() wrongly sends us the exit event).
    // IMP: We don't have to release the memory region because the page mapping (that we did after fork()) will be destroyed anyway
    // after execve() and a new mapping will be created.
    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnExecve() - Entering..." << eol;
        m_logger.debugStream().flush(); 

        // Read the first argument. Both the argument and the contents of the address it points to may have to be changed.
        char path[PATH_MAX] = {0};
        ReadPath(param.m_pid, param.m_userRegs, path);

        if ( _DoOnSyscallCommon(param, path) ) // path indeed changed...
        {
            // Now to the second argument which is of type char**, i.e. a pointer to char*. The first member of this character array 
            // is argv[0], the program name which is the same as the first argument above. Note that what changes here is not the 
            // argument, but contents of client memory. Hence we don't touch any of the user regs.
            UMemContent data;
            data.m_val = 0;

            userreg_t pArg = GetArg2(param.m_userRegs);

            // Loan memory region to store the new path.
            _GetPageRgn(param.m_intercept, param.m_pid, data.m_val);

            // Store the new path value in the page region obtained above.
            WriteMemory(param.m_pid, data.m_val, path);

            // Finally, update the contents of argv[0]. Note that we are not changing pArg (the second argument to execve()); instead,
            // we change the address contained in the first pointer that pArg points to. 
            WriteMemory(param.m_pid, pArg, data.m_strVal, WORD_SIZE);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: sockaddrPtr32 contains the address of sockaddr_in ***in client address space***. This argument is ignored for native (64 bit)
// applications.
void CSyscallHandler::_OnSockCommon(SIParam& param, userreg_t sockaddrPtr32)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSockCommon() - Into " 
            << GetSyscallName(param.m_userRegs) << "()" << eol;
        m_logger.debugStream().flush(); 

        bool isSockcall32 = param.m_flags & F_SOCKCALL32;
        bool isSendmsg = param.m_flags & F_SOCKSENDMSG;

        // NOTE: We don't use reference to addr (compare with other syscall handlers) because we don't modify the address parameter
        // of any socket syscall.
        userreg_t addr = !isSockcall32 ? ((param.m_flags & F_SOCKSENDTO) ? GetArg5(param.m_userRegs) : GetArg2(param.m_userRegs)) 
            : sockaddrPtr32; 

        if ( addr ) // A null value for sockaddr* is valid (at times required!) with sendto()
        {
            // sizeof sockaddr_un > sizeof msghdr, so we use that value as our buffer size. TODO: This could be potentially
            // problematic if sizeof sockaddr_un.sun_family + strlen(sockaddr_un.sun_path) + 1 exceeds 104 (if x86_64) or 108 (if x86)
            // Remember that ptrace() transfers data in word sized chunks. Unlike in other syscalls (where we use our custom allocated
            // memory region to manipulate path), here we are modifying memory region (stack/heap/static variables) not managed by us.
            // sizeof sockaddr_un is 110. 104 and 108 are the next lower (i.e. < 110) multiples of WORD_SIZE for the respective 
            // architectures. A PTRACE_POKEDATA with bigger sizes will overwrite bytes 111 and 112. :-(
            char data[sizeof (sockaddr_un)]; 

            // sockaddr_un (used with UNIX domain sockets) is larger than sockaddr (used with inet sockets). Thus we may end up
            // reading more bytes than required if this is an inet syscall. TODO: Can this pose any problems? 
            ReadMemory(param.m_pid, addr, data, !isSendmsg ? sizeof (sockaddr_un) : sizeof (msghdr));
            !isSockcall32 ? (!isSendmsg ? _DoOnSockCommon64(param, data) : _DoOnSendmsg(param, data)) 
                : _DoOnSockCommon32(param, data, sockaddrPtr32);
        }
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSockCommon() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) 
            << eol;
        m_logger.debugStream().flush(); 
    }
}

void CSyscallHandler::_DoOnSockCommon64(SIParam& param, char* data)
{
    using namespace log4cpp;
    sockaddr_un* pSockAddr = reinterpret_cast<sockaddr_un*> (data);

    // NOTE: We don't use reference to addr (compare with other syscall handlers) because we don't modify the address parameter
    // of any socket syscall.
    userreg_t addr = (param.m_flags & F_SOCKSENDTO) ? GetArg5(param.m_userRegs) : GetArg2(param.m_userRegs); 
    userreg_t& actualSize = (param.m_flags & F_SOCKSENDTO) ? GetArg6(param.m_userRegs) : GetArg3(param.m_userRegs); 
    char* sockName = pSockAddr->sun_path;

    if ( pSockAddr->sun_family == AF_UNIX ) // We are interested only in UNIX domain sockets
    {
        sockName[UNIX_PATH_MAX - 1] = 0; // Just in case
        
        // Abstract sockets have null as the first byte of sun_path and don't appear in the file system.
        if ( !sockName[0] )
        {
            m_logger.infoStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnSockCommon64() - Ignoring abstract socket" 
                "/connection " << &sockName[1] << eol;
            m_logger.infoStream().flush(); 
            return;
        }

        EBaseDir rStatus = _RebuildPath(param.m_pid, param.m_intercept.m_cwd, sockName);

        if ( rStatus == E_NEWOK )
        {
            return;
        }

        if ( rStatus == E_OLDHOME )
        {
            param.m_flags |= F_OLDHOMEDIR;
        }
        else if ( rStatus == E_NEWBAD )
        {
            param.m_flags |= F_BADNEWDIR;
        }

        EPathArg pStatus = _ModifyPath(param.m_pid, sockName, param.m_flags, UNIX_PATH_MAX);

        if ( ShouldModifyArg(pStatus, rStatus) )
        {
            // We don't need _GetPageRgn() here because the member pSockAddr->sun_path is an array (i.e. of fixed, predefined 
            // size). Instead, we have to adjust the third argument of the system call, viz. the actual size of the data 
            // contained in sockaddr_un. Note that actualSize is a reference to the corresponding field in userRegs. 
            actualSize = sizeof pSockAddr->sun_family + strlen(sockName) + 1; // 1 for the null terminator.
            _ModifyPathArg(param.m_pid, param.m_userRegs, addr, data, actualSize);
        }
    }
}

// NOTE: sockaddrPtr32 contains the address of sockaddr_in ***in client address space***. This argument is ignored for native (64 bit)
// applications.
void CSyscallHandler::_DoOnSockCommon32(SIParam& param, char* data, userreg_t sockaddrPtr32)
{
    using namespace log4cpp;

    // pSockAddr is the ***local copy*** of sockaddr_in object
    sockaddr_un* pSockAddr = reinterpret_cast<sockaddr_un*> (data);

    userreg_t pArg = GetArg2(param.m_userRegs);

    // See the comments in _OnSocketcall() above
    userreg_t sizePtr32 = pArg + ((param.m_flags & F_SOCKSENDTO) ? 20 : 8);

    UMemContent content;
    content.m_val = 0;

    // Dereference sizePtr32, i.e. get the value of the size argument.
    ReadMemory(param.m_pid, sizePtr32, content.m_strVal, WORD_SIZE);

    // If running on x86_64, we fetch a word (8 bytes), but 32 bit apps have 4 byte long. Typecast into uint32_t to get the lower 4 
    // bytes.
    uint32_t size = content.m_val;

    char* sockName = pSockAddr->sun_path;

    if ( pSockAddr->sun_family == AF_UNIX ) // We are interested only in UNIX domain sockets
    {
        sockName[UNIX_PATH_MAX - 1] = 0; // Just in case
        
        // Abstract sockets have null as the first byte of sun_path and don't appear in the file system.
        if ( !sockName[0] )
        {
            m_logger.infoStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnSockCommon32() - Ignoring abstract socket" 
                "/connection " << &sockName[1] << eol;
            m_logger.infoStream().flush(); 
            return;
        }

        EBaseDir rStatus = _RebuildPath(param.m_pid, param.m_intercept.m_cwd, sockName);

        if ( rStatus == E_NEWOK )
        {
            return;
        }

        if ( rStatus == E_OLDHOME )
        {
            param.m_flags |= F_OLDHOMEDIR;
        }
        else if ( rStatus == E_NEWBAD )
        {
            param.m_flags |= F_BADNEWDIR;
        }

        EPathArg pStatus = _ModifyPath(param.m_pid, sockName, param.m_flags, UNIX_PATH_MAX);

        if ( ShouldModifyArg(pStatus, rStatus) )
        {
            // Things are different here than any other syscall. We don't change any arguments to socketcall(); what we change is
            // the data referred to by the second argument. Thus, no _ModifyPathArg() since it also updates the registers. Instead,
            // we call WriteMemory() directly.
            size = sizeof pSockAddr->sun_family + strlen(sockName) + 1; // 1 for the null terminator.

            WriteMemory(param.m_pid, sockaddrPtr32, data, size);

#ifdef __x86_64__
            // Remember we had fetched 8 bytes when only 4 were required. We need to put back those extra four bytes.
            // First clear the old value of size (the lower 4 bytes), then update with the new value.
            content.m_val &= (0xfffffffful << 32);
            content.m_val |= size;
#else
            content.m_val = size;
#endif
            WriteMemory(param.m_pid, sizePtr32, content.m_strVal, WORD_SIZE);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSyscallHandler::_OnSyscallCommon(SIParam& param)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon() - Into " 
            << GetSyscallName(param.m_userRegs) << "()" << eol;
        m_logger.debugStream().flush(); 

        char path[PATH_MAX] = {0};
        ReadPath(param.m_pid, param.m_userRegs, path);
        _DoOnSyscallCommon(param, path);
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 

        param.m_intercept._ReleasePageRgn(param.m_pid);

        // It's possible that we may have set the path to empty string earlier for file/dir deletion syscalls. If so, the application
        // will see a return value of -1 even though nothing terrible has happened. We address that problem here.
        if ( param.m_flags & F_DELETE ) 
        {
            WriteUserReg(param.m_pid, GetRetvalOffset(), 0);
        }
    }
}

void CSyscallHandler::_OnSyscallAtCommon(SIParam& param)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallAtCommon() - Into " 
            << GetSyscallName(param.m_userRegs) << "()" << eol;
        m_logger.debugStream().flush(); 

        char path[PATH_MAX] = {0};
        param.m_flags |= F_SYSCALLAT;
        ReadDirPath(param.m_pid, param.m_userRegs, path);
        _DoOnSyscallCommon(param, path);
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallAtCommon() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 

        param.m_intercept._ReleasePageRgn(param.m_pid);

        if ( param.m_flags & F_DELETE ) 
        {
            WriteUserReg(param.m_pid, GetRetvalOffset(), 0);
        }
    }
}

bool CSyscallHandler::_DoOnSyscallCommon(SIParam& param, char* path)
{
    bool retval = false;
    EBaseDir rStatus = _RebuildPath(param.m_pid, param.m_intercept.m_cwd, path);
    
    if ( rStatus == E_NEWOK )
    {
        return retval;
    }

    if ( rStatus == E_OLDHOME )
    {
        param.m_flags |= F_OLDHOMEDIR;
    }
    else if ( rStatus == E_NEWBAD )
    {
        param.m_flags |= F_BADNEWDIR;
    }

    userreg_t& pathAddr = (param.m_flags & F_SYSCALLAT) ? GetArg2(param.m_userRegs) : GetArg1(param.m_userRegs); 

    EPathArg pStatus = _ModifyPath(param.m_pid, path, param.m_flags);

    if ( ShouldModifyArg(pStatus, rStatus) )
    {
        _GetPageRgn(param.m_intercept, param.m_pid, pathAddr);
        _ModifyPathArg(param.m_pid, param.m_userRegs, pathAddr, path);
        retval = true;
    }

    return retval;
}

void CSyscallHandler::_OnSyscallCommon2(SIParam& param, int altSyscall)
{
    using namespace log4cpp;

    // No 'else' part of substance: the return from syscall will be handled by _OnSyscallCommon() instead because of syscall 
    // substitution below.
    if ( param.m_flags & F_ENTRY ) 
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon2() - Into " 
            << GetSyscallName(param.m_userRegs) << "()" << eol;
        m_logger.debugStream().flush(); 
        userreg_t& arg1 = GetArg1(param.m_userRegs); // The function returns a reference

        char path[PATH_MAX] = {0};
        bool exists = true;

        try
        {
            exists = FdToPath(param.m_pid, arg1, path);
        }

        catch ( CException& ex )
        {
            ex << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon2() - Error retrieving path from descriptor. " << ex.StrError();
            throw ex;
        }

        if ( exists )
        {
            _DoOnSyscallCommon2(param, path, altSyscall);
        }
        else
        {
            m_logger.warnStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon2() - Descriptor refers to deleted file." 
                << eol; 
            m_logger.warnStream().flush(); 
        }
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon2() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 
    }
}

void CSyscallHandler::_DoOnSyscallCommon2(SIParam& param, char* path, int altSyscall)
{
    EBaseDir rStatus = _RebuildPath(param.m_pid, param.m_intercept.m_cwd, path);
    
    if ( rStatus == E_NEWOK )
    {
        return;
    }

    if ( rStatus == E_OLDHOME )
    {
        param.m_flags |= F_OLDHOMEDIR;
    }
    else if ( rStatus == E_NEWBAD )
    {
        param.m_flags |= F_BADNEWDIR;
    }

    // Here we dupe the application into executing the alternate system call (eg. chmod() instead of fchmod()) and hence change
    // the first argument from file descriptor to file path. If we were to retain the original syscall, we would have had to
    // create the new file descriptor in the ***target address space*** which means dynamically inserting open() syscall into the
    // application and then retrieving the handle. Replacing the syscall is a better alternative.

    EPathArg pStatus = _ModifyPath(param.m_pid, path, param.m_flags); 

    if ( ShouldModifyArg(pStatus, rStatus) )
    {
        userreg_t& arg1 = GetArg1(param.m_userRegs); // The function returns a reference
        SetSyscallNumXlat(altSyscall, param.m_userRegs);

        // Refer to the comment in _DoOnChdir()
        param.m_intercept._Notify(NOTIFY_ALTSYSCALL, param.m_pid, GetSyscallNumber(param.m_userRegs));

        _GetPageRgn(param.m_intercept, param.m_pid, arg1);
        _ModifyPathArg(param.m_pid, param.m_userRegs, arg1, path);
    }
}

void CSyscallHandler::_OnSyscallCommon3(SIParam& param)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon3() - Into " 
            << GetSyscallName(param.m_userRegs) << "()" << eol;
        m_logger.debugStream().flush(); 

        char oldPath[PATH_MAX] = {0};
        char newPath[PATH_MAX] = {0};

        ReadPaths(param.m_pid, param.m_userRegs, oldPath, newPath);
        _DoOnSyscallCommon3(param, oldPath, newPath);
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallCommon3() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 

        param.m_intercept._ReleasePageRgn(param.m_pid); // Destination
        param.m_intercept._ReleasePageRgn(param.m_pid); // Source
    }
}

void CSyscallHandler::_OnSyscallAtCommon3(SIParam& param)
{
    using namespace log4cpp;

    if ( param.m_flags & F_ENTRY )
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallAtCommon3() - Into " 
            << GetSyscallName(param.m_userRegs) << "()" << eol;
        m_logger.debugStream().flush(); 

        char oldPath[PATH_MAX] = {0};
        char newPath[PATH_MAX] = {0};

        param.m_flags |= F_SYSCALLAT;
        ReadDirPaths(param.m_pid, param.m_userRegs, oldPath, newPath);
        _DoOnSyscallCommon3(param, oldPath, newPath);
    }
    else
    {
        m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_OnSyscallAtCommon3() - Returning from " 
            << GetSyscallName(param.m_userRegs) << "(). Retval: " << int(GetRetStatus(param.m_userRegs)) << eol;
        m_logger.debugStream().flush(); 

        param.m_intercept._ReleasePageRgn(param.m_pid); // Destination
        param.m_intercept._ReleasePageRgn(param.m_pid); // Source
    }
}

void CSyscallHandler::_DoOnSyscallCommon3(SIParam& param, char* oldPath, char* newPath)
{
    using namespace log4cpp;

    m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnSyscallCommon3() - Path: Old: " << oldPath << " New: " 
        << newPath << eol;
    m_logger.debugStream().flush(); 

    BuildFullIf(param.m_intercept.m_cwd, oldPath);
    BuildFullIf(param.m_intercept.m_cwd, newPath);

    Normalise(oldPath);
    Normalise(newPath);

    m_logger.debugStream() << '[' << param.m_pid << "] CSyscallHandler::_DoOnSyscallCommon3() - Full path: Old: " << oldPath 
        << " New: " << newPath << eol;
    m_logger.debugStream().flush(); 

    int syscallNum = GetSyscallNumXlat(param.m_userRegs);
    userreg_t& oldAddr = (param.m_flags & F_SYSCALLAT) ? (syscallNum != SYS_symlinkat ? GetArg2(param.m_userRegs) :
            GetArg1(param.m_userRegs)) : GetArg1(param.m_userRegs);
    userreg_t& newAddr = (param.m_flags & F_SYSCALLAT) ? (syscallNum != SYS_symlinkat ? GetArg4(param.m_userRegs) : 
            GetArg3(param.m_userRegs)) : GetArg2(param.m_userRegs);

    // NOTE: rename() and link() don't work across mount points, hence these calls will fail if the source doesn't already
    // exist on user's virtual machine (this is where the destination file/link will be created).
    // NOTE: The semantics of rename() are different from the shell command mv. If the newPath is a (existing) directory, then 
    // mv moves the oldPath under this directory whereas rename() will fail with error EISDIR.

    // Source path
    _DoOnSyscallCommon3(param, oldAddr, oldPath);

    // Destination path
    // IMP: Clear the F_NOREPLIC flag set by syscall specific handler. This flag is relevant only for the source
    // path handled above. The destination path involves file creation (F_CREATE) as shown below. Also clear the
    // other two flags used for the source path.
    param.m_flags &= ~F_NOREPLIC;
    param.m_flags &= ~F_OLDHOMEDIR;
    param.m_flags &= ~F_BADNEWDIR;
    param.m_flags |= F_CREATE; // IMP!

    _DoOnSyscallCommon3(param, newAddr, newPath);
}

void CSyscallHandler::_DoOnSyscallCommon3(SIParam& param, userreg_t& pathAddr, char* path)
{
    EBaseDir rStatus = _AdjustPathIf(param.m_pid, path);

    if ( rStatus == E_OLDHOME )
    {
        param.m_flags |= F_OLDHOMEDIR;
    }
    else if ( rStatus == E_NEWBAD )
    {
        param.m_flags |= F_BADNEWDIR;
    }

    if ( rStatus != E_NEWOK )
    {
        EPathArg pStatus = _ModifyPath(param.m_pid, path, param.m_flags);

        if ( ShouldModifyArg(pStatus, rStatus) )
        {
            _GetPageRgn(param.m_intercept, param.m_pid, pathAddr);
            _ModifyPathArg(param.m_pid, param.m_userRegs, pathAddr, path);
        }
    }
}
