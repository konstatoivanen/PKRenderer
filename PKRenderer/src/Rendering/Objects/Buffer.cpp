#include "PrecompiledHeader.h"
#include "Buffer.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"

namespace PK::Rendering::Objects
{
    using namespace Utilities;
    using namespace VulkanRHI::Objects;

    Ref<Buffer> Buffer::Create(const BufferLayout& layout, const void* data, size_t count, BufferUsage usage)
    {
        auto api = GraphicsAPI::GetActiveAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanBuffer>(layout, data, count, usage);
        }

        return nullptr;
    }
}