#include "PrecompiledHeader.h"
#include "AccelerationStructure.h"
#include "Rendering/VulkanRHI/Objects/VulkanAccelerationStructure.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Rendering::VulkanRHI::Objects;
    using namespace PK::Rendering::Structs;
    using namespace PK::Utilities;

    Ref<AccelerationStructure> AccelerationStructure::Create(const char* name)
    {
        auto api = GraphicsAPI::GetActiveAPI();
    
        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanAccelerationStructure>(name);
        }
    
        return nullptr;
    }
}
