#include "PrecompiledHeader.h"
#include "PlatformInterfaces.h"
#include <stdio.h>

namespace PK
{
    IPlatformWindowListener::~IPlatformWindowListener() = default;
    PlatformWindow::~PlatformWindow() = default;
    
    void IPlatform::ManagedDeallocate(void* ptr)
    {
        auto head = &s_ManagedAllocations;

        while (*head)
        {
            auto handle = *head;

            if (handle->object == ptr)
            {
                handle->destructor(handle->object);
                *head = handle->next;
                free(handle->object);
                free(handle);
                break;
            }

            head = &handle->next;
        }
    }

    void IPlatform::FatalExit(const char* message)
    {
        // Assume message wasnt consumed if it reached this point.
        if (message)
        {
            PK_PLATFORM_DEBUG_BREAK;
            printf("// FATAL UNCONSUMED EXIT BEGIN //\nUser message:%s\n// FATAL UNCONSUMED EXIT END//\n", message);
            fflush(stdout);
        }

        auto head = s_ManagedAllocations;

        while (head)
        {
            auto handle = head;
            handle->destructor(handle->object);
            head = head->next;
            free(handle->object);
            free(handle);
        }

        PK::Platform::Terminate();

        exit(-1);
    }

    void IPlatform::PollEvents() 
    { 
        Platform::PollEvents(false); 
    }
    
    void IPlatform::WaitEvents() 
    {
        Platform::PollEvents(true); 
    }

    void IPlatform::AddManagedAllocation(void* object, void(*destructor)(void*))
    {
        auto alloc = static_cast<ManagedAllocation*>(malloc(sizeof(ManagedAllocation)));
        alloc->destructor = destructor;
        alloc->object = object;
        alloc->next = nullptr;

        auto head = &s_ManagedAllocations;

        while (*head)
        {
            head = &(*head)->next;
        }

        *head = alloc;
    }
}
