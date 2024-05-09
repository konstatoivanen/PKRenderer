#include "PrecompiledHeader.h"
#include "BindArray.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanBindArray.h"
#include "Rendering/RHI/Driver.h"

namespace PK::Rendering::RHI::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::RHI::Vulkan::Objects;

    template<> BindArray<Texture>::~BindArray() = default;
    template<> BindArray<Buffer>::~BindArray() = default;

    template<>
    BindArrayRef<Texture> BindArray<Texture>::Create(size_t capacity)
    {
        auto api = Driver::Get()->GetAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanBindArray>(capacity);
            default: return nullptr;
        }
    }

    template<>
    BindArrayRef<Buffer> BindArray<Buffer>::Create(size_t capacity)
    {
        auto api = Driver::Get()->GetAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanBindArray>(capacity);
            default: return nullptr;
        }
    }
}