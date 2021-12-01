#include "PrecompiledHeader.h"
#include "Buffer.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Rendering::VulkanRHI::Objects;

    Ref<Buffer> Buffer::Create(BufferUsage usage, const BufferLayout& layout, size_t count)
    {
        auto api = GraphicsAPI::GetActiveAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanBuffer>(usage, layout, count);
        }

        return nullptr;
    }
}