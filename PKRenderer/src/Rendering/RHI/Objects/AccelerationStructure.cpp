#include "PrecompiledHeader.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanAccelerationStructure.h"
#include "Rendering/RHI/Driver.h"
#include "AccelerationStructure.h"

namespace PK::Rendering::RHI::Objects
{
    using namespace PK::Rendering::RHI::Vulkan::Objects;
    using namespace PK::Utilities;

    AccelerationStructureRef AccelerationStructure::Create(const char* name)
    {
        auto api = Driver::Get()->GetAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanAccelerationStructure>(name);
        }

        return nullptr;
    }
}
