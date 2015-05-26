#ifndef _CExecState_
#define _CExecState_

#include "CState.h"

class CProcCtrl;

class CExecState : public CState
{
    public:
        CExecState(CProcCtrl* pProcCtrl);
        virtual ~CExecState();
        virtual void Execute(pid_t pid, int sigNum);
        virtual void Notify(int code, int data);
};

#endif
