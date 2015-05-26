#ifndef _CDefState_
#define _CDefState_

#include "CState.h"

class CProcCtrl;

class CDefState : public CState
{
    public:
        CDefState(CProcCtrl* pProcCtrl);
        virtual ~CDefState();
        virtual void Execute(pid_t pid, int sigNum);
        virtual void Notify(int code, int data);
};

#endif
