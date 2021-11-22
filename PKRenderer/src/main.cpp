#include "PrecompiledHeader.h"

#ifdef PK_DEBUG
	#define _CRTDBG_MAP_ALLOC  
	#include <stdlib.h>  
	#include <crtdbg.h>  
#endif 

#include "Core/Application.h"

int main(int argc, char** argv)
{
#ifdef PK_DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	auto app = new PK::Core::Application("PK Renderer");
	app->Run();
	delete app;
}