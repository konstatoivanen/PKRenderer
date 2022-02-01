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
#endif

    auto app = new PK::Core::Application({ argc, argv }, "PK Renderer");
    app->Run();
    delete app;
}