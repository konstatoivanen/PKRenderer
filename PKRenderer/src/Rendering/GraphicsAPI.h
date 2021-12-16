#pragma once
#include "Core/NoCopy.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "Utilities/Ref.h"

namespace PK::Rendering
{
    using namespace Structs;
    using namespace Utilities;
    using namespace PK::Rendering::Objects;

    struct GraphicsDriver : public PK::Core::NoCopy
    {
        virtual ~GraphicsDriver() = default;
        virtual APIType GetAPI() const = 0;
        virtual CommandBuffer* GetPrimaryCommandBuffer() = 0;
        virtual void WaitForIdle() const = 0;
        virtual size_t GetMemoryUsageKB() const = 0;
        virtual void GC() = 0;

        static Scope<GraphicsDriver> Create(APIType api);
    };

    namespace GraphicsAPI
    {
        GraphicsDriver* GetActiveDriver();
        
        template<typename T>
        inline T* GetActiveDriver() 
        {
            static_assert(std::is_base_of<GraphicsDriver, T>::value, "Template argument type does not derive from GraphicsDriver!"); 
            return static_cast<T*>(GetActiveDriver()); 
        }

        APIType GetActiveAPI();
        
        // Add support for secondary commandbuffers
        CommandBuffer* GetCommandBuffer();

        size_t GetMemoryUsageKB();

        void GC();
    }
}