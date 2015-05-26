#include "Defs.h"
#include "CState.h"

#include <log4cpp/Category.hh>

CState::CState(CProcCtrl* pProcCtrl) : m_pProcCtrl(pProcCtrl), m_logger(log4cpp::Category::getInstance(LOG_CATEGORY))
{
}

CState::~CState()
{
}
