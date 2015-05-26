#ifndef _CSyscallHandler_
#define _CSyscallHandler_

#include <sys/types.h>
#include <regex.h>
#include <pthread.h>
#include <string.h>
#include <string>
#include <ext/hash_map>

#include "Defs.h"
#include "SyscallDefs.h"
#include "SyscallTypeDefs.h"

struct user_regs_struct;
struct SIParam;
class CIntercept;
class CSyscallHandler;

namespace log4cpp
{
    class Category;
}

typedef void (CSyscallHandler::*syshandler_t)(SIParam&);

class CSyscallHandler
{
    public:
        static CSyscallHandler* GetInstance();
        static void Destroy();
        syshandler_t Get(int syscall);

    private:
        void _OnOpen(SIParam& param);
        void _OnStat(SIParam& param);
        void _OnFstat(SIParam& param);
        void _OnLstat(SIParam& param);
        void _OnAccess(SIParam& param);
        void _OnConnect(SIParam& param);
        void _OnSendto(SIParam& param);
        void _OnSendmsg(SIParam& param);
        void _OnBind(SIParam& param);
        void _OnClone(SIParam& param);
        void _OnExecve(SIParam& param);
        void _OnSocketcall(SIParam& param); // Used by 32 bit apps
        void _OnTruncate(SIParam& param);
        void _OnChdir(SIParam& param);
        void _OnFchdir(SIParam& param);
        void _OnRename(SIParam& param);
        void _OnMkdir(SIParam& param);
        void _OnRmdir(SIParam& param);
        void _OnCreat(SIParam& param);
        void _OnLink(SIParam& param);
        void _OnUnlink(SIParam& param);
        void _OnSymlink(SIParam& param);
        void _OnReadlink(SIParam& param);
        void _OnChmod(SIParam& param);
        void _OnFchmod(SIParam& param);
        void _OnChown(SIParam& param);
        void _OnFchown(SIParam& param);
        void _OnLchown(SIParam& param);
        void _OnUtime(SIParam& param);
        void _OnMknod(SIParam& param);
        void _OnAcct(SIParam& param);
        void _OnSetxattr(SIParam& param);
        void _OnLsetxattr(SIParam& param);
        void _OnFsetxattr(SIParam& param);
        void _OnGetxattr(SIParam& param);
        void _OnLgetxattr(SIParam& param);
        void _OnFgetxattr(SIParam& param);
        void _OnListxattr(SIParam& param);
        void _OnLlistxattr(SIParam& param);
        void _OnFlistxattr(SIParam& param);
        void _OnRemovexattr(SIParam& param);
        void _OnLremovexattr(SIParam& param);
        void _OnFremovexattr(SIParam& param);
        void _OnUtimes(SIParam& param);
        void _OnOpenat(SIParam& param);
        void _OnMkdirat(SIParam& param);
        void _OnMknodat(SIParam& param);
        void _OnFchownat(SIParam& param);
        void _OnFutimesat(SIParam& param);
        void _OnFstatat(SIParam& param);
        void _OnUnlinkat(SIParam& param);
        void _OnRenameat(SIParam& param);
        void _OnLinkat(SIParam& param);
        void _OnSymlinkat(SIParam& param);
        void _OnReadlinkat(SIParam& param);
        void _OnFchmodat(SIParam& param);
        void _OnFaccessat(SIParam& param);
        void _OnUtimensat(SIParam& param);

    private:
        void _DoOnOpen(SIParam& param, char* path);
        void _DoOnChdir(SIParam& param, char* path, int altSyscall);
        void _DoOnSendmsg(SIParam& param, char* path);
        void _DoOnExecve(SIParam& param);
        void _OnSockCommon(SIParam& param, userreg_t sockaddrPtr32 = 0);
        void _DoOnSockCommon64(SIParam& param, char* path);
        void _DoOnSockCommon32(SIParam& param, char* path, userreg_t sockaddrPtr32);
        void _OnSyscallCommon(SIParam& param);
        void _OnSyscallAtCommon(SIParam& param);
        bool _DoOnSyscallCommon(SIParam& param, char* path);
        void _OnSyscallCommon2(SIParam& param, int altSyscall);
        void _DoOnSyscallCommon2(SIParam& param, char* path, int altSyscall);
        void _OnSyscallCommon3(SIParam& param);
        void _OnSyscallAtCommon3(SIParam& param);
        void _DoOnSyscallCommon3(SIParam& param, char* oldPath, char* newPath);
        void _DoOnSyscallCommon3(SIParam& param, userreg_t& pathAddr, char* path);

    private:
        CSyscallHandler();
        ~CSyscallHandler();
        CSyscallHandler(const CSyscallHandler&);
        CSyscallHandler(const CSyscallHandler*);
        bool _IsShlib(const char* path);
        bool _InNewRootDir(const char* path);
        void _GetPageRgn(CIntercept& intercept, pid_t pid, userreg_t& pathAddr);
        void _InitHashMap();
        EBaseDir _RebuildPath(pid_t pid, const std::string& cwd, char* path);
        EBaseDir _AdjustPathIf(pid_t pid, char* path);
        void _RebuildDirPath(pid_t pid, const std::string& cwd, char* path);
        EPathArg _ModifyPath(pid_t pid, char* path, int flags, int limit = PATH_MAX_LEN);
        bool _ModifyDirPath(pid_t pid, char* path);
        void _ModifyPathArg(pid_t pid, user_regs_struct& userRegs, userreg_t pathAddr, char* data, int size = 0);

    private:
        static CSyscallHandler* m_pSelf;
        const std::string& m_newRootDir;
        std::string m_oldHomeDir;
        pthread_mutex_t m_fileopMtx;
        regex_t m_shlibRegex;
        __gnu_cxx::hash_map<int, syshandler_t> m_syscallHash;
        log4cpp::Category& m_logger;
};

inline bool CSyscallHandler::_InNewRootDir(const char* path)
{
    return ( strstr(path, m_newRootDir.c_str()) == path );
}

#endif
