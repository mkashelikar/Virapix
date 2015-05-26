#ifndef _IState_
#define _IState_

#include <sys/types.h>

class IState
{
    public:
        virtual void Execute(pid_t pid, int sigNum) = 0;
        virtual void Notify(int code, int data) = 0;
};

#endif
