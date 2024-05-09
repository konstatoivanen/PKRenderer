#include "PrecompiledHeader.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanBuffer.h"
#include "Rendering/RHI/Driver.h"
#include "Buffer.h"

namespace PK::Rendering::RHI::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::RHI::Vulkan::Objects;

    Buffer::~Buffer() = default;

    BufferRef Buffer::Create(size_t size, BufferUsage usage, const char* name)
    {
        auto api = Driver::Get()->GetAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanBuffer>(size, usage, name);
            default: return nullptr;
        }
    }
}