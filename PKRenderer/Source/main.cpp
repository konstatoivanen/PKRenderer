#include "PrecompiledHeader.h"

#ifdef PK_DEBUG
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif 

#include "Core/IApplication.h"

// Set preference for high performance NVIDIA or AMD GPU in a multi GPU context.
#if defined(PK_PLATFORM_WINDOWS)
extern "C" 
{
    PK_DLLEXPORT uint32_t NvOptimusEnablement = 0x00000001;
    PK_DLLEXPORT int32_t AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char** argv)
{
#if defined(_CRTDBG_MAP_ALLOC)
    auto flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF;
    _CrtSetDbgFlag(flag);
   // _crtBreakAlloc = 483;
#endif

    auto platformDriver = PK::Platform::CreateDriver();

    if (platformDriver)
    {
        auto application = PK::CreateProjectApplication({ argv, argc });

        application->Execute();

        PK::FreeProjectApplication(application);
    }

    PK::Platform::DestroyDriver(platformDriver);
}