#ifndef _CApp_
#define _CApp_

#include <sys/types.h>
#include <signal.h>
#include <string>
#include <list>

namespace log4cpp
{
    class Category;
}

struct SConfig;

class ICtrl;

class CApp
{
    public:
        // Need a better alternative: the default arguments allow calling GetInstance() from functions deep inside the application
        // where there's no access to command line arguments. Of course, this assumes that main() has properly constructed the CApp
        // object passing it the correct values. In other words, subsequent callers of GetInstance() "know" that the object does 
        // exist and that the function will simply return a pointer to that object. There's no portable way to access argv, argc
        // and envp from functions other than main() (other than propagating the pointers of course!)
        static CApp* GetInstance(int argc = 1, char** argv = 0, char** envp = 0);
        static void Destroy();
        void Run();

        unsigned long GetMmapOffset(bool is64Bit) const;
        unsigned long GetSetpgidOffset(bool is64Bit) const;
        const std::string& GetLdsoName() const;
        const std::string& GetLibcName() const;
        const std::string& GetNewRootDir() const;

    private:
        static CApp* m_pSelf;
        int m_argc;
        char** m_argv;
        char** m_envp;

        bool m_ok;
#ifdef __x86_64__
        unsigned long m_mmapOffset64;
        unsigned long m_setpgidOffset64;
#endif
        unsigned long m_mmapOffset32;
        unsigned long m_setpgidOffset32;
        std::string m_ldso;
        std::string m_libc;
        std::string m_newRootDir;
        log4cpp::Category& m_logger;

        // It's unsafe to use auto_ptr with STL containers :-(
        typedef std::list<ICtrl*> ICtrlList;
        ICtrlList m_iCtrlList;

    private:
        CApp(int argc, char** argv, char** envp);
        CApp(const CApp&);
        CApp(const CApp*);
        ~CApp();
        static void _SigHandler(int, siginfo_t*, void*);
        void _RegisterICtrl(siginfo_t*);

        void _InstallSigHandler();
        void _InitLogger(const SConfig& config);
        void _SetEnv();
#ifdef __x86_64__
        void _Get32BitOffset();
#endif
        void _Run();
        void _WaitForCompletion(ICtrl* pTop);
};

#ifdef __x86_64__
inline unsigned long CApp::GetMmapOffset(bool is64Bit) const
{
    return is64Bit ? m_mmapOffset64 : m_mmapOffset32;
}

inline unsigned long CApp::GetSetpgidOffset(bool is64Bit) const
{
    return is64Bit ? m_setpgidOffset64 : m_setpgidOffset32;
}
#else
inline unsigned long CApp::GetMmapOffset(bool unused) const
{
    return m_mmapOffset32;
}

inline unsigned long CApp::GetSetpgidOffset(bool unused) const
{
    return m_setpgidOffset32;
}
#endif

inline const std::string& CApp::GetLdsoName() const
{
    return m_ldso;
}

inline const std::string& CApp::GetLibcName() const
{
    return m_libc;
}

inline const std::string& CApp::GetNewRootDir() const
{
    return m_newRootDir;
}

#endif
