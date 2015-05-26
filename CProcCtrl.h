#ifndef _CProcCtrl_
#define _CProcCtrl_

#include "ICtrl.h"
#include "IState.h"
#include "CThread.h"
#include "ProcCtrlDefs.h"

#include <stdlib.h>
#include <string>
#include <map>
#include <memory>

struct SUserRegs;

namespace log4cpp
{
    class Category;
}

class CProcCtrl : public ICtrl
{
    friend class CThread<CProcCtrl>;
    friend class CDefState;
    friend class CStopState;
    friend class CSyscallState;
    friend class CForkState;
    friend class CCloneState;
    friend class CExecState;

    public:
        CProcCtrl(pid_t pid, bool is64Bit, bool registerSelf = true);
        virtual ~CProcCtrl();
        virtual pid_t GetPid() const;
        virtual void Run();
        virtual int Wait();
        virtual int Terminate();

    private:
        unsigned char m_flags;
        pid_t m_pid;
        CThread<CProcCtrl> m_thread;
        IState* m_pCurState;
        std::map<int, IState*> m_states;
        std::auto_ptr<SUserRegs> m_pRegsBackup;
        log4cpp::Category& m_logger;

    private:
        void _Is64Bit(bool is64Bit);
        bool _Is64Bit() const;
        bool _IsGrpLeader() const;
        int _Wait();
        void _NotifyParent(int sig);
        void _CreateStates();
        void _RegisterSelf();
        void _PrepareAttach();
        void _Attach();
        void* _Start(void* arg); // Thread procedure used internally by CThread (Tight coupling?)
        void _SetCurState(bool isStopped, int waitStatus);
        void _Execute(pid_t pid, int sigNum);
        void _PreInsertSyscall(int syscall);
        void _SyscallInserted(int syscall);
        void _SyscallReturn(int syscall);
        void _KillProcess();
        void _DestroyStates();
};

inline pid_t CProcCtrl::GetPid() const
{
    return abs(m_pid);
}

inline void CProcCtrl::_Is64Bit(bool is64Bit)
{
#ifdef __x86_64__
    if ( is64Bit )
    {
        m_flags |= F_PROC64BIT;
    }
    else
    {
        m_flags &= ~F_PROC64BIT;
    }
#endif
}

inline bool CProcCtrl::_Is64Bit() const
{
#ifdef __x86_64__
    return m_flags & F_PROC64BIT;
#else
    return false;
#endif
}

inline bool CProcCtrl::_IsGrpLeader() const
{
    return m_pid < 0;
}

inline int CProcCtrl::_Wait()
{
    return m_thread.SigWait();
}

inline void CProcCtrl::_NotifyParent(int sig)
{
    m_thread.NotifyParent(sig);
}

#endif
