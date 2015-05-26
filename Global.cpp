#include "Global.h"
#include "Defs.h"
#include "CElfReader.h"
#include "ProcUtil.h"
#include "FileUtil.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

const std::string& LTrim(std::string& str)
{
    int count = 0;
    int length = str.size();

    while ( count < length )
    {
        if ( !isspace(str[count]) )
        {
            break;
        }

        ++count;
    }

    str.erase(0, count);
    return str;
}

char* LTrim(char* str)
{
    if ( !str || !*str )
    {
        return str;
    }

    int count = -1;
    int length = strlen(str);

    while ( isspace(str[++count]) );

    if ( !count )
    {
        return str;
    }

    char* ptr = str + count; 
    length -= count;

    for ( count = 0; count < length; ++count )
    {
        str[count] = ptr[count];
    }

    str[count] = 0;
    return str;
}

char* RTrim(char* str)
{
    if ( !str || !*str )
    {
        return str;
    }

    char* ptr = str + strlen(str);

    while ( isspace(*--ptr) );

    ptr[1] = 0;
    return str;
}

void Prepend(const char* prefix, char* str)
{
    int prefixLen = strlen(prefix);
    int strLen = strlen(str);
    char* pStrEnd = str + strLen - 1;
    char* pStrNewEnd = pStrEnd + prefixLen;

    while ( pStrEnd >= str )
    {
        *pStrNewEnd-- = *pStrEnd--;
    }

    while ( *prefix )
    {
        *++pStrEnd = *prefix++;
    }

    str[prefixLen + strLen] = 0;
}

void RemoveDup(char* str, char ch)
{
    for ( char* ptr1 = str; *ptr1; )
    {
        if ( *ptr1 == ch && *(ptr1 + 1) == ch )
        {
            for ( char* ptr2 = ptr1; *ptr2; ++ptr2 )
            {
                *ptr2 = *(ptr2 + 1);
            }
        }
        else
        {
            ++ptr1;
        }
    }
}

void Remove(char* str, int count, int start)
{
    char* ptr1 = str + start;
    char* ptr2 = ptr1 + count;

    while ( *ptr2 )
    {
        *ptr1++ = *ptr2++;
    }

    *ptr1 = 0;
}

const std::string& GetLastError(std::string& str)
{
    char errStr[ERRMSG_MAX_LEN];
    return str = strerror_r(errno, errStr, ERRMSG_MAX_LEN - 1); // Thread-safe alternative to strerror()
}

const std::string& GetLastError(std::string& str, int error)
{
    char errStr[ERRMSG_MAX_LEN];
    return str = strerror_r(error, errStr, ERRMSG_MAX_LEN - 1); // Thread-safe alternative to strerror()
}
