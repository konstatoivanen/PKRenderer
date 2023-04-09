#include "PrecompiledHeader.h"
#include "BindArray.h"
#include "Rendering/VulkanRHI/Objects/VulkanBindArray.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::VulkanRHI::Objects;

    template<>
    Ref<BindArray<Texture>> BindArray<Texture>::Create(size_t capacity)
    {
        auto api = GraphicsAPI::GetActiveAPI();

        switch (api)
        {
        case APIType::Vulkan: return CreateRef<VulkanBindArray>(capacity);
        }

        return nullptr;
    }

    template<>
    Ref<BindArray<Buffer>> BindArray<Buffer>::Create(size_t capacity)
    {
        auto api = GraphicsAPI::GetActiveAPI();

        switch (api)
        {
        case APIType::Vulkan: return CreateRef<VulkanBindArray>(capacity);
        }

        return nullptr;
    }
}