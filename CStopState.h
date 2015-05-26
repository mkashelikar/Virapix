#ifndef _CStopState_
#define _CStopState_

#include "CState.h"
#include "SyscallTypeDefs.h"

class CProcCtrl;

class CStopState : public CState
{
    public:
        CStopState(CProcCtrl* pProcCtrl);
        virtual ~CStopState();
        virtual void Execute(pid_t pid, int sigNum);
        virtual void Notify(int code, int data);

    private:
        bool m_optSet;

    private:
        int _Wait();
        void _ConfigNewProcess(pid_t pid);
        void _InsertSyscall(pid_t pid, int syscall);
        void _PrepareChangePGID(pid_t pid);
        void _PreparePageReserve(pid_t pid);
        void _SetRegs(pid_t pid, userreg_t syscallAddr, int syscall);
};

#endif
