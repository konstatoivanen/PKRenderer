#include "PrecompiledHeader.h"

#ifdef PK_DEBUG
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif 

#include "Core/IApplication.h"

int main(int argc, char** argv)
{
#if defined(PK_DEBUG) && defined(_WIN32)
    auto flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF;
    _CrtSetDbgFlag(flag);
   //_crtBreakAlloc = 68461;
#endif

    auto application = PK::CreateProjectApplication({ argc, argv });
    application->Execute();
    delete application;
}