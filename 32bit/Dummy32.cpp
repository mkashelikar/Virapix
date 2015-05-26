#include <signal.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    kill(getppid(), SIGUSR1);

    sigset_t set;
    sigemptyset(&set);
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, 0);

    while ( true )
    {
        siginfo_t sigInfo;
        sigwaitinfo(&set, &sigInfo);
        
        if ( sigInfo.si_pid == getppid() && sigInfo.si_signo == SIGUSR1 )
        {
            break;
        }
    }

    return 0;
}

