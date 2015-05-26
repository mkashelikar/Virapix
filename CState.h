#ifndef _CState_
#define _CState_

#include "IState.h"

#include <sys/types.h>

class CProcCtrl;

namespace log4cpp
{
    class Category;
}

class CState : public IState
{
    public:
        CState(CProcCtrl* pProcCtrl);
        virtual ~CState();

    public:
        virtual void Execute(pid_t pid, int sigNum) = 0;
        virtual void Notify(int code, int data) = 0;

    protected:
        CProcCtrl* m_pProcCtrl;
        log4cpp::Category& m_logger;
};

#endif
