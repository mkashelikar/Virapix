#ifndef _ProcUtil_
#define _ProcUtil_

#include <sys/types.h>
#include <string>

enum EShLib
{
    E_LIBC,
    E_LDSO,
    E_NULL
};

enum EMemMapPerm
{
    E_R,
    E_W,
    E_X
};

struct SMemMap
{
    unsigned long m_start;
    unsigned long m_end;
    unsigned long m_offset;
    unsigned long m_size;
    std::string m_dev;
    std::string m_path;
};

void EnumToPartialName(EShLib eTag, std::string& partialName);
void GetMemMap(pid_t pid, EShLib lib, SMemMap& memMap, EMemMapPerm constraint = E_X);
void GetMemMap(pid_t pid, const std::string& module, SMemMap& memMap, EMemMapPerm constraint = E_X);
const std::string& GetExePath(pid_t pid, std::string& path);
bool FdToPath(pid_t pid, int fd, char* path);

inline bool CheckPerm(EMemMapPerm constraint, const std::string& permissions)
{
    return constraint == E_R ? ( permissions.find("r--") != std::string::npos ? true : false ) : ( constraint == E_W ? 
           ( permissions[1] == 'w' ? true : false ) : constraint == E_X ? ( permissions[2] == 'x' ? true : false ) : false );
}

#endif
