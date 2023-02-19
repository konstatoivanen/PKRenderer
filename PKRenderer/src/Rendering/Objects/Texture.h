#pragma once
#include "Utilities/NativeInterface.h"
#include "Core/Services/AssetDatabase.h"
#include "Rendering/Structs/Descriptors.h"

namespace PK::Rendering::Objects
{
    class Texture : public Core::Services::Asset, public Core::Services::IAssetImportSimple, public Utilities::NativeInterface<Texture>
    {
        friend Utilities::Ref<Texture> Core::Services::AssetImporters::Create();

        public:
            static Utilities::Ref<Texture> Create(const Structs::TextureDescriptor& descriptor, const char* name);

            Texture(const char* name) : m_name(name){}
            virtual ~Texture() = default;
            void Import(const char* filepath) override final;
            virtual void SetSampler(const Structs::SamplerDescriptor& sampler) = 0;
            virtual bool Validate(const Math::uint3& resolution) = 0;
            virtual bool Validate(const uint32_t levels, const uint32_t layers) = 0;
            virtual bool Validate(const Structs::TextureDescriptor& descriptor) = 0;

            constexpr const Structs::TextureUsage GetUsage() const { return m_descriptor.usage; }
            constexpr const bool IsConcurrent() const { return (m_descriptor.usage & Structs::TextureUsage::Concurrent) != 0; }
            constexpr const Structs::SamplerDescriptor& GetSamplerDescriptor() const { return m_descriptor.sampler; }
            constexpr const Math::uint4 GetRect() const { return { 0, 0, m_descriptor.resolution.x, m_descriptor.resolution.y }; }
            constexpr const Math::uint3 GetResolution() const { return m_descriptor.resolution; }
            constexpr const uint32_t GetLevels() const { return m_descriptor.levels; }
            constexpr const uint32_t GetLayers() const { return m_descriptor.layers; }

        protected:
            Structs::TextureDescriptor m_descriptor;
            std::string m_name = "Texture";
    };
}