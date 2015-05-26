#ifndef _ICtrl_
#define _ICtrl_

#include <sys/types.h>

class ICtrl
{
    public:
        virtual pid_t GetPid() const = 0;
        virtual void Run() = 0;
        virtual int Wait() = 0;
        virtual int Terminate() = 0;
};

#endif
