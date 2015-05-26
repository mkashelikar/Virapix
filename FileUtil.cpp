#include "Global.h"
#include "FileUtil.h"
#include "CException.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <string>

// Fast copying of files using Linux's "zero-copy" technique. We eliminate kernel buffer to user buffer transfers so typical of 
// read()/write() calls. We use const char* because the arguments will be passed down to us in the form of C style array by our
// caller. For efficiency reasons, we avoid replicating the buffer.
void CopyFile(const char* from, const char* to)
{
    int pd[2];
    bool isErr = true;

    if ( !pipe(pd) )
    {
        int fdFrom = -1;
        int fdTo = -1;
        struct stat toInfo;

        fdFrom = open(from, O_RDONLY);

        if ( stat(to, &toInfo) ) // The file doesn't exist.
        {
            fdTo = open(to, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
        else
        { 
            if ( S_ISDIR(toInfo.st_mode) ) // The target is a directory: copy "from" to this directory.
            {
                std::string strTo = to;
                strTo += '/';
                strTo += basename(from);
                fdTo = open(strTo.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            }
        }

        if ( fdFrom != -1 && fdTo != -1 )
        {
            loff_t fromOffset1 = 0;
            loff_t toOffset2 = 0;
            ssize_t bytesRead = splice(fdFrom, &fromOffset1, pd[1], 0, PAGE_SIZE, 0);
            ssize_t bytesWritten = 0;

            while ( bytesRead && bytesRead != -1 && bytesWritten != -1 )
            {
                bytesWritten = splice(pd[0], 0, fdTo, &toOffset2, PAGE_SIZE, 0);   
                bytesRead = splice(fdFrom, &fromOffset1, pd[1], 0, PAGE_SIZE, 0);
            }

            // WARNING: May not contain the right error code if both splice() calls in the loop fail.
            if ( bytesRead != -1 && bytesWritten != -1 )
            {
                isErr = false;
            }
        }

        if ( fdFrom != -1 )
        {
            close(fdFrom);
        }

        if ( fdTo != -1 )
        {
            close(fdTo);
        }

        close(pd[0]);
        close(pd[1]);
    }

    if ( isErr )
    {
        CException ex(errno);
        ex << errMsg;
        throw ex;
    }
}

// The function removes "..", "." and trailing slashes from an ***absolute*** path and converts it into a normal representation. The 
// code looks ugly primarily because we have to include cases like "/..", "/../", "/.", "/./", "/usr/.." etc. 
void Normalise(char* path)
{
    RemoveDup(path, '/');

    char* ptr1 = 0;
    char* ptr2 = path;
    int len = strlen(path);
    int offset = 0;

    while ( ptr1 = strchr(ptr2, '/') )
    {
        if ( ptr1[1] == '.' )
        {
            if ( ptr1[2] == '.' && (ptr1[3] == '/' || !ptr1[3]) ) // Path contains ".."
            {
                if ( ptr1 != path )
                {
                    *ptr1 = 0;
                    ptr2 = strrchr(path, '/');
                    *ptr1 = '/';
                    offset = ptr2 != path ? 0 : ptr1[3] ? 0 : 1;
                }
                else
                {
                    ptr2 = ptr1;
                    offset = ptr1[3] ? 0 : 1;
                }

                len -= 3;
                memmove(ptr2 + offset, ptr1 + 3, len - (ptr1 - path) + 1);
                path[len + offset] = 0;
            }
            else if ( ptr1[2] == '/' || !ptr1[2] ) // Path contains "."
            {
                offset = ptr1 != path ? 0 : ptr1[2] ? 0 : 1;
                len -= 2;
                ptr2 = ptr1;
                memmove(ptr2 + offset, ptr1 + 2, len - (ptr1 - path) + 1);
                path[len + offset] = 0;
            }
            else
            {
                ptr2 = ptr1 + 1;
            }
        }
        else
        {
            ptr2 = ptr1 + 1;
        }
    }

    len = strlen(path);
    ((len > 1) && (path[len - 1] == '/')) && (path[len - 1] = 0);
}
