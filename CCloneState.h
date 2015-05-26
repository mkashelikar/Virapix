#ifndef _CCloneState_
#define _CCloneState_

#include "CState.h"

class CProcCtrl;

class CCloneState : public CState
{
    public:
        CCloneState(CProcCtrl* pProcCtrl);
        virtual ~CCloneState();
        virtual void Execute(pid_t pid, int sigNum);
        virtual void Notify(int code, int data);
};

#endif
