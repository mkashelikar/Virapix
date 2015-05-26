#ifndef _CAutoMutex_
#define _CAutoMutex_

#include <pthread.h>

class CAutoMutex
{
    public:
        explicit CAutoMutex(pthread_mutex_t& mutex) : m_mutex(mutex)
        {
            pthread_mutex_lock(&m_mutex);
        }

        ~CAutoMutex()
        {
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t& m_mutex;
};

#endif
