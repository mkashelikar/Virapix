#ifndef _Functor_
#define _Functor_

#include <sys/types.h>

template <typename T, typename A1 = int, typename A2 = int, typename A3 = int>
class CFunc3Args
{
    typedef void (T::*FPtr)(A1, A2, A3);

    public:
        CFunc3Args(T* pObj, FPtr pFunc) : m_pObj(pObj), m_pFunc(pFunc) { }
        void operator()(A1 arg1, A2 arg2, A3 arg3) { (m_pObj->*m_pFunc)(arg1, arg2, arg3); }

    private:
        T* m_pObj;
        FPtr m_pFunc;
};

class CSyscallState;
typedef CFunc3Args<CSyscallState, int, pid_t> CSig1;

#endif
