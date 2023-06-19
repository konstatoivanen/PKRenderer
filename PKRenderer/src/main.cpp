#include "PrecompiledHeader.h"

#ifdef PK_DEBUG
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif 

#include "Core/Application.h"

int main(int argc, char** argv)
{
#if defined(PK_DEBUG) && defined(_WIN32)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
   // _crtBreakAlloc(49891);
#endif

    auto application = new PK::Core::Application({ argc, argv }, "PK Renderer");
    application->Execute();
    delete application;
}