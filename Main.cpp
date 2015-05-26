#include "CApp.h"
#include "CSysCompat.h"
#include "CSyscallHandler.h"
#include "CException.h"

int main(int argc, char** argv, char** envp)
{
    CApp* pApp = CApp::GetInstance(argc, argv, envp); // CApp ***must*** be created before anything else!
    CSysCompat::GetInstance(); // No need to store the returned pointer!
    CSyscallHandler::GetInstance(); // No need to store the returned pointer!

    pApp->Run();

    CSyscallHandler::Destroy();
    CSysCompat::Destroy();
    CApp::Destroy();

    return 0;
}
