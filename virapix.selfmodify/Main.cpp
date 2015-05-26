#include "Main.h"
#include "Defs.h"
#include "Global.h"
#include "CApp.h"
#include "CSysCompat.h"
#include "CSyscallHandler.h"
#include "CException.h"

#include <unistd.h>
#include <iostream>

char devMountPt[PATH_MAX_LEN] = ""; // Initialise the array to ensure that space is allocated for it in the binary

int main(int argc, char** argv, char** envp)
{
    using namespace std;

    char optStr[] = { '+', ':', OPT_GETMNTPT, OPT_SETMNTPT, ':', 0 };
    char opt;

    opterr = 0;

    while ( ( opt = getopt(argc, argv, optStr) ) != -1 )
    {
        switch ( opt )
        {
            case OPT_GETMNTPT:
                cout << devMountPt << flush;
                return 0;

            case OPT_SETMNTPT:
            {
                try
                {
                    SetMountPoint(optarg);
                }

                catch ( const CException& ex )
                {
                    cout << "main() - Error setting mount point " << optarg << " - " << ex.str() << endl << flush;
                    return -1;
                }

                return 0;
            }

            case ':':
                cout << "Missing argument for option -" << char(optopt) << endl << flush;
                return -1;

            case '?':
                cout << "Invalid option -" << char(optopt) << endl << flush;
                return -1;
        }

    }

    if ( argc < 2 )
    {
        cout << "Usage: appvirt <program name> [program arguments]\n" << flush;
        return -1;
    }

    Start(argc, argv, envp);
    return 0;
}

void Start(int argc, char** argv, char** envp)
{
    CApp* pApp = CApp::GetInstance(argc, argv, envp); // CApp ***must*** be created before anything else!
    CSysCompat::GetInstance(); // No need to store the returned pointer!
    CSyscallHandler::GetInstance(); // No need to store the returned pointer!

    pApp->Run();

    CSyscallHandler::Destroy();
    CSysCompat::Destroy();
    CApp::Destroy();
}
