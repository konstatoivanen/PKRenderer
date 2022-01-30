#include "PrecompiledHeader.h"
#include "Texture.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/GraphicsAPI.h"

using namespace PK::Core;
using namespace PK::Core::Services;
using namespace PK::Utilities;
using namespace PK::Rendering;
using namespace PK::Rendering::Objects;
using namespace PK::Rendering::VulkanRHI::Objects;

Ref<Texture> PK::Rendering::Objects::Texture::Create(const TextureDescriptor& descriptor)
{
    auto api = GraphicsAPI::GetActiveAPI();

    switch (api)
    {
        case APIType::Vulkan: return CreateRef<VulkanTexture>(descriptor);
    }

    return nullptr;
}

template<>
bool AssetImporters::IsValidExtension<Texture>(const std::filesystem::path& extension) { return extension.compare(".ktx2") == 0; }

template<>
Ref<Texture> AssetImporters::Create()
{ 
    auto api = GraphicsAPI::GetActiveAPI();

    switch (api)
    {
        case APIType::Vulkan: return CreateRef<VulkanTexture>();
    }

    return nullptr; 
}
