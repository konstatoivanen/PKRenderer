#include "PrecompiledHeader.h"
#include "Buffer.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"

namespace PK::Rendering::Objects
{
    using namespace Utilities;
    using namespace VulkanRHI::Objects;

    Ref<Buffer> Buffer::Create(const BufferLayout& layout, const void* data, size_t count, BufferUsage usage, const char* name)
    {
        auto api = GraphicsAPI::GetActiveAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanBuffer>(layout, data, count, usage, name);
        }

        return nullptr;
    }

    void Buffer::SetData(const void* data, size_t offset, size_t size)
    {
        auto dst = BeginWrite(0, GetCapacity());
        memcpy(reinterpret_cast<char*>(dst) + offset, data, size);
        EndWrite();
    }

    void Buffer::SetSubData(const void* data, size_t offset, size_t size)
    {
        auto dst = BeginWrite(offset, size);
        memcpy(dst, data, size);
        EndWrite();
    }
}