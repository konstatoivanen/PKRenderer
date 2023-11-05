#include "PrecompiledHeader.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanBuffer.h"
#include "Rendering/RHI/Driver.h"
#include "Buffer.h"

namespace PK::Rendering::RHI::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::RHI::Vulkan::Objects;

    BufferRef Buffer::Create(const BufferLayout& layout, size_t count, BufferUsage usage, const char* name)
    {
        auto api = Driver::Get()->GetAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanBuffer>(layout, count, usage, name);
        }

        return nullptr;
    }
}