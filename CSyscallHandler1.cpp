#include "SyscallGlobal.h"
#include "FileUtil.h"
#include "CSyscallHandler.h"
#include "CApp.h"
#include "CSysCompat.h"
#include "CAutoMutex.h"
#include "CException.h"

#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <errno.h>
#include <assert.h>

#include <log4cpp/Category.hh>

CSyscallHandler* CSyscallHandler::m_pSelf = 0;

//////////////////////////////////////////////////////// Inline methods ///////////////////////////////////////////////////////

inline bool InOldHomeDir(const char* path, const char* homeDir)
{
    return strstr(path, homeDir) == path;
}

//////////////////////////////////////////////////////// Public methods ///////////////////////////////////////////////////////

CSyscallHandler::CSyscallHandler() : m_newRootDir(CApp::GetInstance()->GetNewRootDir()), 
    m_oldHomeDir(getenv("HOME")), m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{
    m_oldHomeDir.erase(0, m_newRootDir.size());
    _InitHashMap();
    pthread_mutex_init(&m_fileopMtx, 0);

    // Store the compiled text regular expression SHLIB_REGEX in m_shlibRegex
    regcomp(&m_shlibRegex, SHLIB_REGEX, REG_EXTENDED | REG_NOSUB);
}

// Dummy private copy constructors only to prevent copying of CSyscallHandler objects. Never used.
CSyscallHandler::CSyscallHandler(const CSyscallHandler&) : m_newRootDir(CApp::GetInstance()->GetNewRootDir()), 
    m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{ 
}

CSyscallHandler::CSyscallHandler(const CSyscallHandler*) : m_newRootDir(CApp::GetInstance()->GetNewRootDir()),
    m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{ 
}

CSyscallHandler::~CSyscallHandler()
{
    regfree(&m_shlibRegex);

    // The structure of the program ensures that the destructor is called only after the termination of all contending threads. In 
    // effect, there won't be any thread holding the mutex when the object is being destroyed. 
    pthread_mutex_destroy(&m_fileopMtx);
}

CSyscallHandler* CSyscallHandler::GetInstance()
{
    return m_pSelf ? m_pSelf : (m_pSelf = new CSyscallHandler());
}

void CSyscallHandler::Destroy()
{
    delete m_pSelf;
    m_pSelf = 0;
}

syshandler_t CSyscallHandler::Get(int syscall)
{
    using namespace __gnu_cxx;

    hash_map<int, syshandler_t>::iterator iter = m_syscallHash.find(syscall);
    return iter != m_syscallHash.end() ? iter->second : syshandler_t(0);
}

//////////////////////////////////////////////////////// Private methods ///////////////////////////////////////////////////////

void CSyscallHandler::_InitHashMap()
{
    m_syscallHash[SYS_open] = &CSyscallHandler::_OnOpen;
    m_syscallHash[SYS_stat] = &CSyscallHandler::_OnStat;
    m_syscallHash[SYS_fstat] = &CSyscallHandler::_OnFstat;
    m_syscallHash[SYS_lstat] = &CSyscallHandler::_OnLstat;
    m_syscallHash[SYS_access] = &CSyscallHandler::_OnAccess;
    m_syscallHash[SYS_connect] = &CSyscallHandler::_OnConnect;
    m_syscallHash[SYS_sendto] = &CSyscallHandler::_OnSendto;
    m_syscallHash[SYS_sendmsg] = &CSyscallHandler::_OnSendmsg;
    m_syscallHash[SYS_bind] = &CSyscallHandler::_OnBind;
    m_syscallHash[SYS_clone] = &CSyscallHandler::_OnClone;
    m_syscallHash[SYS_execve] = &CSyscallHandler::_OnExecve;
    m_syscallHash[sys32::SYS_SOCKETCALL] = &CSyscallHandler::_OnSocketcall; // Used by 32 bit applications
    m_syscallHash[SYS_truncate] = &CSyscallHandler::_OnTruncate;
    m_syscallHash[SYS_chdir] = &CSyscallHandler::_OnChdir;
    m_syscallHash[SYS_fchdir] = &CSyscallHandler::_OnFchdir;
    m_syscallHash[SYS_rename] = &CSyscallHandler::_OnRename;
    m_syscallHash[SYS_mkdir] = &CSyscallHandler::_OnMkdir;
    m_syscallHash[SYS_rmdir] = &CSyscallHandler::_OnRmdir;
    m_syscallHash[SYS_creat] = &CSyscallHandler::_OnCreat;
    m_syscallHash[SYS_link] = &CSyscallHandler::_OnLink;
    m_syscallHash[SYS_unlink] = &CSyscallHandler::_OnUnlink;
    m_syscallHash[SYS_symlink] = &CSyscallHandler::_OnSymlink;
    m_syscallHash[SYS_readlink] = &CSyscallHandler::_OnReadlink;
    m_syscallHash[SYS_chmod] = &CSyscallHandler::_OnChmod;
    m_syscallHash[SYS_fchmod] = &CSyscallHandler::_OnFchmod;
    m_syscallHash[SYS_chown] = &CSyscallHandler::_OnChown;
    m_syscallHash[SYS_fchown] = &CSyscallHandler::_OnFchown;
    m_syscallHash[SYS_lchown] = &CSyscallHandler::_OnLchown;
    m_syscallHash[SYS_utime] = &CSyscallHandler::_OnUtime;
    m_syscallHash[SYS_mknod] = &CSyscallHandler::_OnMknod;
    m_syscallHash[SYS_acct] = &CSyscallHandler::_OnAcct;
    m_syscallHash[SYS_setxattr] = &CSyscallHandler::_OnSetxattr;
    m_syscallHash[SYS_lsetxattr] = &CSyscallHandler::_OnLsetxattr;
    m_syscallHash[SYS_fsetxattr] = &CSyscallHandler::_OnFsetxattr;
    m_syscallHash[SYS_getxattr] = &CSyscallHandler::_OnGetxattr;
    m_syscallHash[SYS_lgetxattr] = &CSyscallHandler::_OnLgetxattr;
    m_syscallHash[SYS_fgetxattr] = &CSyscallHandler::_OnFgetxattr;
    m_syscallHash[SYS_listxattr] = &CSyscallHandler::_OnListxattr;
    m_syscallHash[SYS_llistxattr] = &CSyscallHandler::_OnLlistxattr;
    m_syscallHash[SYS_flistxattr] = &CSyscallHandler::_OnFlistxattr;
    m_syscallHash[SYS_removexattr] = &CSyscallHandler::_OnRemovexattr;
    m_syscallHash[SYS_lremovexattr] = &CSyscallHandler::_OnLremovexattr;
    m_syscallHash[SYS_fremovexattr] = &CSyscallHandler::_OnFremovexattr;
    m_syscallHash[SYS_utimes] = &CSyscallHandler::_OnUtimes;
    m_syscallHash[SYS_openat] = &CSyscallHandler::_OnOpenat;
    m_syscallHash[SYS_mkdirat] = &CSyscallHandler::_OnMkdirat;
    m_syscallHash[SYS_mknodat] = &CSyscallHandler::_OnMknodat;
    m_syscallHash[SYS_fchownat] = &CSyscallHandler::_OnFchownat;
    m_syscallHash[SYS_futimesat] = &CSyscallHandler::_OnFutimesat;
    m_syscallHash[SYS_newfstatat] = &CSyscallHandler::_OnFstatat;
    m_syscallHash[SYS_unlinkat] = &CSyscallHandler::_OnUnlinkat;
    m_syscallHash[SYS_renameat] = &CSyscallHandler::_OnRenameat;
    m_syscallHash[SYS_linkat] = &CSyscallHandler::_OnLinkat;
    m_syscallHash[SYS_symlinkat] = &CSyscallHandler::_OnSymlinkat;
    m_syscallHash[SYS_readlinkat] = &CSyscallHandler::_OnReadlinkat;
    m_syscallHash[SYS_fchmodat] = &CSyscallHandler::_OnFchmodat;
    m_syscallHash[SYS_faccessat] = &CSyscallHandler::_OnFaccessat;
    m_syscallHash[SYS_utimensat] = &CSyscallHandler::_OnUtimensat;
}

EBaseDir CSyscallHandler::_RebuildPath(pid_t pid, const std::string& cwd, char* path)
{
    using namespace log4cpp;

    m_logger.debugStream() << '[' << pid << "] CSyscallHandler::_RebuildPath() - Path: " << path << eol;
    m_logger.debugStream().flush(); 

    BuildFullIf(cwd, path);

    // Remove "..", "." and also any trailing '/'
    Normalise(path);

    m_logger.debugStream() << '[' << pid << "] CSyscallHandler::_RebuildPath() - Full path: " << path << eol;
    m_logger.debugStream().flush(); 

    return _AdjustPathIf(pid, path);
}

EBaseDir CSyscallHandler::_AdjustPathIf(pid_t pid, char* path)
{
    using namespace log4cpp;

    EBaseDir retval = E_OLD;

    if ( _InNewRootDir(path) ) // The path is rooted at the new location...
    {
        struct stat unused;

        // lstat() for dummy symbolic links that point to non-existent files (Firefox creates one such link for the lock file). stat() 
        // returns -1 for such symbolic link whereas lstat() returns 0 which is what we want.

        if ( lstat(path, &unused) ) // ...but is invalid.
        {
            m_logger.noticeStream() << '[' << pid << "] CSyscallHandler::_AdjustPathIf() - Non existent path: stripping off new root component."
                << eol;
            m_logger.noticeStream().flush(); 
            Remove(path, m_newRootDir.size()); // Strip off the new root.

            if ( !path[0] ) // If path contains only the new root, Remove() leaves it empty!
            {
                path[0] = '/';
                path[1] = 0;
            }

            retval = InOldHomeDir(path, m_oldHomeDir.c_str()) ? E_OLDHOME : E_NEWBAD;
        }
        else
        {
            retval = E_NEWOK;
        }
    }
    else
    {
        if ( InOldHomeDir(path, m_oldHomeDir.c_str()) )
        {
            retval = E_OLDHOME;
        }
    }

    return retval;
}

void CSyscallHandler::_RebuildDirPath(pid_t pid, const std::string& cwd, char* path)
{
    using namespace log4cpp;

    m_logger.debugStream() << '[' << pid << "] CSyscallHandler::_RebuildDirPath() - Path: " << path << eol;
    m_logger.debugStream().flush(); 

    BuildFullIf(cwd, path);

    // Remove "..", "." and also any trailing '/'
    Normalise(path);

    m_logger.debugStream() << '[' << pid << "] CSyscallHandler::_RebuildDirPath() - Full path: " << path << eol;
    m_logger.debugStream().flush(); 
}

// A return value of false means no further processing of path is required i.e., the original path argument to the syscall be used
// as is without any manipulation.
bool CSyscallHandler::_ModifyDirPath(pid_t pid, char* path)
{
    using namespace log4cpp;

    assert(*path == '/');
    bool isModif = false;

    if ( _InNewRootDir(path) )
    {
        Remove(path, m_newRootDir.size());

        if ( !path[0] ) // If path contains only the new root, Remove() will leave it empty!
        {
            path[0] = '/';
            path[1] = 0;
        }

        isModif = true;
    }

    // We first check whether the directory exists on the host system. If not, we search the new location.
    if ( access(path, F_OK) ) 
    {
        Prepend(m_newRootDir.c_str(), path); 
        isModif = !access(path, F_OK);
    }

    m_logger.infoStream() << '[' << pid << "] CSyscallHandler::_ModifyDirPath() - New path set to " << path << eol;
    m_logger.infoStream().flush();

    return isModif;
}

EPathArg CSyscallHandler::_ModifyPath(pid_t pid, char* path, int flags, int limit)
{
    using namespace log4cpp;

    EPathArg retval = E_BADLEN;

    assert(*path == '/');
    CAutoMutex autoMtx(m_fileopMtx);

    // Create new directories and replicate path only if it's less than PATH_MAX_LEN bytes long.
    if ( (m_newRootDir.size() + strlen(path)) < limit )
    {
        retval = CreateDirTree(m_newRootDir, path, flags);

        m_logger.infoStream() << '[' << pid << "] CSyscallHandler::_ModifyPath() - New path set to " << path << eol;
        m_logger.infoStream().flush();
    }
    else
    {
        *path = 0;
        m_logger.errorStream() << '[' << pid << "] CSyscallHandler::_ModifyPath() - Path too long: truncating to zero..." << eol;
        m_logger.errorStream().flush();
    }

    return retval;
}

// Even though pathAddr refers back to some field in userRegs, we pass it separately because the position of the path argument (and
// consequently the corresponding field in userRegs ) may not be the same across system calls.
void CSyscallHandler::_ModifyPathArg(pid_t pid, user_regs_struct& userRegs, userreg_t pathAddr, char* data, int size)
{
    // Write the new path into the client address. Thus the system call will now execute with a modified value of its path
    // argument.
    if ( data )
    {
        WriteMemory(pid, pathAddr, data, size);
    }

    if ( ptrace(PTRACE_SETREGS, pid, 0, &userRegs) == -1 )
    {
        CException ex(errno);
        ex << '[' << pid << "] CSyscallHandler::_ModifyPathArg() - " << errMsg;
        throw ex;
    }
}
