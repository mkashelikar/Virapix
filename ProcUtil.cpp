#include "Global.h"
#include "ProcUtil.h"
#include "ProcUtilDefs.h"
#include "CException.h"

#include <sys/stat.h>
#include <gnu/libc-version.h>
#include <linux/limits.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <string>
#include <fstream>

// HACK: This function assumes the library names to be ld-<libc version>.so and libc-<libc version>.so
void EnumToPartialName(EShLib shLibTag, std::string& partialName)
{
    using namespace std;

    string libcVersion = gnu_get_libc_version();
    libcVersion += ".so";

    switch ( shLibTag )
    {
        case E_LIBC:
            partialName = LIBC_PATTERN;
            partialName += libcVersion;
            break;

        case E_LDSO:
            partialName = LDSO_PATTERN;
            partialName += libcVersion;
            break;

        default:
            break;
    }

    return;
}

// HACK: This function assumes a certain layout of /proc/<pid>/maps file
void GetMemMap(pid_t pid, const std::string& module, SMemMap& memMap, EMemMapPerm constraint)
{
    using namespace std;

    stringstream procMapsPath;
    ifstream procMaps;
    string partialName;
    string permissions;
    char hyphen;
    string modulePath;
    unsigned long pos;

#ifdef _DEBUG
    ofstream diskOut("mapsAll.txt", ios::app);
    diskOut << "\n\t\t\t******************[" << pid << "]**************************************************\n" << flush;
#endif

    procMapsPath << PROC_DIR << '/' << pid << '/' << PROC_MAPS_FILE;
    procMaps.open(procMapsPath.str().c_str());

    if ( procMaps )
    {
        while ( procMaps )
        {
            procMaps >> hex >> memMap.m_start >> hyphen >> memMap.m_end >> permissions >> memMap.m_offset >> memMap.m_dev >> 
                memMap.m_size;

            // We use getline() for the path field because:
            // 1. The field may be empty (in case of anonymous mappings)
            // 2. There may be additional data beyond the field (for instance the string "(deleted)" in case of shared memory mapping)
            // The extraction operator (>>) fails to handle these situations.
            getline(procMaps, memMap.m_path);

#ifdef _DEBUG
            diskOut << memMap.m_start << hyphen << memMap.m_end << ' ' << memMap.m_path << endl << flush;
#endif
            if ( CheckPerm(constraint, permissions) && ( memMap.m_path.find(module) != string::npos ) )
            {
                LTrim(memMap.m_path);
                break;
            }
        }
    }
    else
    {
        throw CException(ERR_MAPSFILE);
    }

    if ( !procMaps )
    {
#ifdef _DEBUG
        diskOut << "FAILED!\n" << flush;
#endif
        throw CException(ERR_SHLIBNOMAP);
    }
}

void GetMemMap(pid_t pid, EShLib lib, SMemMap& memMap, EMemMapPerm constraint)
{
    using namespace std;

    string partialName;

    EnumToPartialName(lib, partialName);
    GetMemMap(pid, partialName, memMap, constraint);
}

const std::string& GetExePath(pid_t pid, std::string& path)
{
    using namespace std;

    stringstream procExePath;
    char _path[PATH_MAX] = {0};

    path = "";
    procExePath << PROC_DIR << '/' << pid << '/' << PROC_EXE_FILE;

    int pathLen = readlink(procExePath.str().c_str(), _path, PATH_MAX - 1);

    if ( pathLen == -1 )
    {
        CException ex(errno);
        ex << errMsg;
        throw ex;
    }

    _path[pathLen] = 0;
    return path = _path; 
}

bool FdToPath(pid_t pid, int fd, char* path)
{
    using namespace std;

    *path = 0;

    stringstream procFdDir;
    bool retval = true;

    // We call getpgid() because pid may also refer to a thread. And /proc has entry only for the process, not threads. It obviously
    // follows that this function presupposes that the controlling process is the group leader.
    procFdDir << PROC_DIR << '/' << getpgid(pid) << '/' << PROC_FD_DIR;

    DIR* pDir = opendir(procFdDir.str().c_str());

    if ( !pDir )
    {
        CException ex(errno);
        ex << errMsg;
        throw ex;
    }

    dirent direntStruct;
    dirent* pDirent = 0;

    string symlinkPath = procFdDir.str() + '/';
    const int POS = symlinkPath.size();

    stringstream strFd;
    strFd << fd;

    char target[PATH_MAX];
    int bytes = 0;
    int status = readdir_r(pDir, &direntStruct, &pDirent); // Reentrant alternative to readdir()

    while ( !status && pDirent )
    {
        symlinkPath += pDirent->d_name;

        if ( strFd.str() == pDirent->d_name )
        {
            if ( (bytes = readlink(symlinkPath.c_str(), target, PATH_MAX - 1)) == -1 )
            {
                CException ex(errno);
                ex << errMsg;
                throw ex;
            }

            target[bytes] = 0;
            strcpy(path, target);
            break;
        }

        status = readdir_r(pDir, &direntStruct, &pDirent);
        symlinkPath.erase(POS);
    }

    if ( *path ) // Search successful!
    {
        if ( access(path, F_OK) ) // The file doesn't exist
        {
            retval = false;
        }
    }
    else
    {
        retval = false;
    }

    closedir(pDir);
    return retval;
}
