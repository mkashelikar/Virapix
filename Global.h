#ifndef _Global_
#define _Global_

#include <string>
#include <string.h>

const std::string& LTrim(std::string& str);
char* LTrim(char* str);
char* RTrim(char* str);
void Prepend(const char* prefix, char* str);
void RemoveDup(char* str, char ch);
void Remove(char* str, int count, int start = 0);
const std::string& GetLastError(std::string& str);
const std::string& GetLastError(std::string& str, int error);

inline char* Trim(char* str)
{
    LTrim(str);
    RTrim(str);
    return str;
}

#endif
