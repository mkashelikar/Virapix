#ifndef _CIntercept_
#define _CIntercept_

#include "InterceptDefs.h"
#include "SyscallTypeDefs.h"
#include "Functor.h"

#include <stdint.h>
#include <string>

struct user_regs_struct;
class CSyscallHandler;

namespace log4cpp
{
    class Category;
}

class CIntercept
{
    friend class CSyscallHandler;

    public:
        CIntercept(CSig1 sig1);
        ~CIntercept();

        void SetPageStart(userreg_t pageStart);
        void OnSyscall(pid_t pid, user_regs_struct& userRegs, bool entryExit);

    private:
        pid_t m_pageSlots[NUM_SLOTS];
        int8_t m_firstFreeSlot;
        std::string m_cwd;
        std::string m_cwdOnHold;
        userreg_t m_pageStart;
        log4cpp::Category& m_logger;

    private:
        userreg_t _GetPageRgn(pid_t pid);
        void _ReleasePageRgn(pid_t pid);
        CSig1 _Notify;
};

#endif
