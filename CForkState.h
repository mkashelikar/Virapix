#ifndef _CForkState_
#define _CForkState_

#include "CState.h"

class CProcCtrl;

class CForkState : public CState
{
    public:
        CForkState(CProcCtrl* pProcCtrl);
        virtual ~CForkState();
        virtual void Execute(pid_t pid, int sigNum);
        virtual void Notify(int code, int data);

    private:
        void _OnFork(pid_t pid, pid_t childPid);
        int _Wait();
};

#endif
