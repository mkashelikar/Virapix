#ifndef _CSyscallState_
#define _CSyscallState_

#include "SyscallTypeDefs.h"
#include "CState.h"
#include "CIntercept.h"

#include <map>

struct user_regs_struct;
class CProcCtrl;

class CSyscallState : public CState
{
    public:
        CSyscallState(CProcCtrl* pProcCtrl);
        virtual ~CSyscallState();
        virtual void Execute(pid_t pid, int);
        virtual void Notify(int code, int data);

    private:
        int m_insertedSyscall;
        std::map<pid_t, int> m_sysCallers;
        CIntercept m_intercept;

    private:
        bool _IsInsertSyscallExit(bool entryExit);
        void _NotifyParent(int sig);
        bool _ToggleSyscallState(pid_t pid, int sysCall);
        void _SyscallUpdate(int code, pid_t pid, int newSyscall);
        void _OnInsertSyscall(pid_t pid, user_regs_struct& userRegs);
        void _OnSyscallReturn(pid_t pid, userreg_t status);
        void _OnSetpgidReturn(pid_t pid);
        void _OnMmapReturn(pid_t pid, userreg_t status);
};

#endif
