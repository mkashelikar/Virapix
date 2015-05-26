#ifndef _CThread_
#define _CThread_

#include "Defs.h"
#include "CException.h"
#include <pthread.h>
#include <signal.h>
#include <assert.h>

template <typename T>
class CThread
{
    public:
        CThread(T* pCtrl, void* pArg = 0) : m_pCtrl(pCtrl), m_pArg(pArg)
        {
            pthread_sigmask(0, 0, &m_sigMask);

            assert(sigismember(&m_sigMask, SIGWAKEPARENT0) == 1);
            assert(sigismember(&m_sigMask, SIGWAKEPARENT1) == 1);

            // Remember, the constructor is called from the parent (creator) thread
            m_creator = pthread_self();
        }

        ~CThread()
        {
            if ( IsRunning() )
            {
                Kill();
            }
        }

        void Run()
        {
            int error = pthread_create(&m_self, 0, _Run, this); // pthread functions return errno instead of -1

            if ( error )
            {
                CException ex(error);
                ex << errMsg;
                throw ex;
            }
        }
        
        bool IsRunning()
        {
            return !pthread_kill(m_self, 0);
        }

        int SigWait()
        {
            int sigNum = 0;
            sigwait(&m_sigMask, &sigNum);
            return sigNum;
        }

        void NotifyParent(int sigNum)
        {
            pthread_kill(m_creator, sigNum);
        }

        int Join()
        {
            return pthread_join(m_self, 0);
        }

        int Kill()
        {
            return pthread_cancel(m_self);
        }

    private:
        T* m_pCtrl;
        void* m_pArg; // Argument to be passed to T::_Start()
        pthread_t m_self;
        pthread_t m_creator; // The thread that spawned us. Note that there's no parent/child relationship in POSIX threads!
        sigset_t m_sigMask;

    private:
        static void* _Run(void* pArg)
        {
            CThread* pThis = static_cast<CThread*> (pArg); 
            return pThis->m_pCtrl->_Start(pThis->m_pArg);
        }
};

#endif
