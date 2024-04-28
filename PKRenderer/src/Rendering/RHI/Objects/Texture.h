#pragma once
#include "Utilities/NativeInterface.h"
#include "Core/Assets/Asset.h"
#include "Rendering/RHI/Structs.h"

namespace PK::Rendering::RHI::Objects
{
    typedef Utilities::Ref<class Texture> TextureRef;

    class Texture : public Core::Assets::AssetWithImport<>, public Utilities::NativeInterface<Texture>
    {
        public:
            static TextureRef Create(const TextureDescriptor& descriptor, const char* name);
            static bool Validate(TextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name);

            Texture(const char* name) : m_name(name){}
            virtual ~Texture() = default;
            void AssetImport(const char* filepath) final;
            virtual void SetSampler(const SamplerDescriptor& sampler) = 0;
            virtual bool Validate(const Math::uint3& resolution) = 0;
            virtual bool Validate(const uint32_t levels, const uint32_t layers) = 0;
            virtual bool Validate(const TextureDescriptor& descriptor) = 0;

            constexpr const TextureUsage GetUsage() const { return m_descriptor.usage; }
            constexpr const bool IsConcurrent() const { return (m_descriptor.usage & TextureUsage::Concurrent) != 0; }
            constexpr const bool IsTracked() const { return (m_descriptor.usage & TextureUsage::ReadOnly) == 0; }
            constexpr const SamplerDescriptor& GetSamplerDescriptor() const { return m_descriptor.sampler; }
            constexpr const Math::uint4 GetRect() const { return { 0, 0, m_descriptor.resolution.x, m_descriptor.resolution.y }; }
            constexpr const Math::uint3 GetResolution() const { return m_descriptor.resolution; }
            constexpr const uint32_t GetLevels() const { return m_descriptor.levels; }
            constexpr const uint32_t GetLayers() const { return m_descriptor.layers; }

        protected:
            TextureDescriptor m_descriptor;
            std::string m_name = "Texture";
    };
}