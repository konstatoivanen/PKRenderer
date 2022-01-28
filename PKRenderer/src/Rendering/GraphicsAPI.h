#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "Utilities/Ref.h"

namespace PK::Rendering
{
    using namespace Structs;
    using namespace Utilities;
    using namespace PK::Rendering::Objects;

    // @TODO investigate if this is can be fulfilled by a possible dx12 implementation?
    struct DriverMemoryInfo
    {
        uint32_t blockCount;
        uint32_t allocationCount;
        uint32_t unusedRangeCount;
        size_t usedBytes;
        size_t unusedBytes;
        size_t allocationSizeMin;
        size_t allocationSizeAvg;
        size_t allocationSizeMax;
        size_t unusedRangeSizeMin;
        size_t unusedRangeSizeAvg; 
        size_t unusedRangeSizeMax;
    };

    struct GraphicsDriver : public PK::Core::NoCopy
    {
        virtual ~GraphicsDriver() = default;
        virtual APIType GetAPI() const = 0;
        virtual CommandBuffer* GetPrimaryCommandBuffer() = 0;
        virtual void WaitForIdle() const = 0;
        virtual DriverMemoryInfo GetMemoryInfo() const = 0;
        virtual size_t GetBufferOffsetAlignment(BufferUsage usage) const = 0;
        virtual void GC() = 0;

        static Scope<GraphicsDriver> Create(const std::string& workingDirectory, APIType api);
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

        DriverMemoryInfo GetMemoryInfo();
        size_t GetBufferOffsetAlignment(BufferUsage usage);

        void GC();
    }
}