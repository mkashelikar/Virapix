#include "Defs.h"
#include "CException.h"

#include <errno.h>
#include <string.h>

#include <log4cpp/Category.hh>

//////////////////////////////////////////////// Manipulator ////////////////////////////////////////////////

std::ostream& errMsg(std::ostream& os)
{
    CException& ex = static_cast<CException&> (os);
    return os << ex.StrError();
}

//////////////////////////////////////////////// Member functions  ////////////////////////////////////////////////

CException::CException(int errNum)
{
    if ( errNum )
    {
        char strError[ERRMSG_MAX_LEN];
        m_strError = strerror_r(errNum, strError, ERRMSG_MAX_LEN - 1); // Thread-safe alternative to strerror()
    }
}

CException::CException(const std::string& strError) : m_strError(strError)
{
}

CException::CException(const CException& ex)
{
    str(ex.str());
    m_strError = ex.StrError();
}
