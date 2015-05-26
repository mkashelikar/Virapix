#include "SIParam.h"
#include "SyscallGlobal.h"
#include "CIntercept.h"
#include "CSyscallHandler.h"
#include "CApp.h"

#include <sys/types.h>
#include <string.h>
#include <string>

#include <log4cpp/Category.hh>

CIntercept::CIntercept(CSig1 sig1) : m_pageStart(0), m_firstFreeSlot(0), m_cwd(CApp::GetInstance()->GetNewRootDir()),
    _Notify(sig1), m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{
    memset(m_pageSlots, 0, (sizeof (pid_t)) * NUM_SLOTS);
}

CIntercept::~CIntercept()
{
}

void CIntercept::SetPageStart(userreg_t pageStart)
{
    m_pageStart = pageStart;

    // Seems redundant (we already do this in the constructor), but is essential if execve() was the last syscall intercepted.
    // This syscall never returns and thus we don't get an opportunity to release the slots in the handler function. 
    memset(m_pageSlots, 0, (sizeof (pid_t)) * NUM_SLOTS);
}

void CIntercept::OnSyscall(pid_t pid, user_regs_struct& userRegs, bool entryExit)
{
    using namespace log4cpp;

    // See the comment in SyscallGlobal.h to understand the purpose of the boolean flag ignore.
    bool ignore = false;
    CSyscallHandler* pHandler = CSyscallHandler::GetInstance();
    userreg_t syscall = GetSyscallNumXlat(userRegs, &ignore);
    syshandler_t HandlerProc = !ignore ? pHandler->Get(syscall) : 0;

    if ( HandlerProc )
    {
        SIParam param(*this, pid, userRegs, entryExit ? F_EXIT : F_ENTRY);
        (pHandler->*HandlerProc)(param);
    }
#if 1
    else
    {
        if ( !entryExit )
        {
            m_logger.debugStream() << '[' << pid << "] CIntercept::OnSyscall() - Entering " << GetSyscallName(userRegs) << "()" 
                << eol;
        }
        else
        {
            m_logger.debugStream() << '[' << pid << "] CIntercept::OnSyscall() - Exiting " << GetSyscallName(userRegs) << 
                "(). Retval: " << long(GetRetStatus(userRegs)) << eol;
        }

        m_logger.debugStream().flush();
    }
#endif
}

// We divide the reserved page into NUM_SLOTS areas each of PAGE_SLOT_SIZE capacity. Each area holds the modified path of a
// file that is opened for writing. The idea is to accommodate multiple threads of execution simultaneously calling file related 
// system calls. 
userreg_t CIntercept::_GetPageRgn(pid_t pid)
{
    assert(m_pageStart);

    userreg_t address = (userreg_t) -1;

    if ( m_firstFreeSlot == NUM_SLOTS )
    {
        // Admittedly there are unnecessary loops when we start with 0 each time, especially since it's quite clear that first few 
        // slots will always be taken. There's obviously room for improvement, for example having another variable say m_lastUsedSlot
        // which when used in conjunction with m_firstFreeSlot will help eliminate those extra loops. However attractive the idea
        // seems, it's far from perfect due to following reasons:
        // 1. The value of NUM_SLOTS isn't so large as to incur heavy penalty. Also there aren't many situations in which we'll be
        // required to loop NUM_SLOTS number of times.
        // 2. The alternative logic discussed above requires some not-so-elegant code to deal with "holes", i.e. free slots
        // interspersed with occupied ones given the fact that interleaved system calls from multiple threads may not exit in the
        // same order in which they were invoked. 
        for ( m_firstFreeSlot = 0; m_firstFreeSlot < NUM_SLOTS;  ++m_firstFreeSlot )
        {
            if ( !m_pageSlots[m_firstFreeSlot] )
            {
                m_pageSlots[m_firstFreeSlot] = pid; // Mark the slot as taken by the process pid
                address = m_pageStart + (PAGE_SLOT_SIZE * m_firstFreeSlot);
                m_firstFreeSlot = NUM_SLOTS; // We don't know yet where the next free slot lives
                break;
            }
        }
    }
    else
    {
        m_pageSlots[m_firstFreeSlot] = pid; // Mark the slot as taken by the process pid
        address = m_pageStart + (PAGE_SLOT_SIZE * m_firstFreeSlot);
        m_firstFreeSlot = NUM_SLOTS; // We don't know yet where the next free slot lives
    }

    return address;
}

void CIntercept::_ReleasePageRgn(pid_t pid)
{
    for ( int idx = 0; idx < NUM_SLOTS; ++idx )
    {
        // The pid may or may not be present, both being valid cases.
        if ( m_pageSlots[idx] == pid )
        {
            m_pageSlots[idx] = 0; // Mark the slot as free

            if ( idx < m_firstFreeSlot )
            {
                m_firstFreeSlot = idx;
            }

            break;
        }
    }
}
