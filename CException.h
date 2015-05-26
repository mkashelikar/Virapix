#ifndef _CException_
#define _CException_

#include <string>
#include <sstream>

std::ostream& errMsg(std::ostream& os);

class CException : public std::stringstream
{
    public:
        CException(int err = 0);
        explicit CException(const std::string& strErr);
        CException(const CException& ex);
        const std::string& StrError() const;

        // The stupid str() member function returns a copy, not a reference of the underlying string object.
        operator std::string() const; 

    private:
        std::string m_strError;
};

inline const std::string& CException::StrError() const
{ 
    return m_strError;
}

inline CException::operator std::string() const
{
    return str();
}

#endif
