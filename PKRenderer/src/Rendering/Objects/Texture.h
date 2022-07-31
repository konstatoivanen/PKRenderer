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

            virtual ~Texture() = default;
            virtual void SetData(const void* data, size_t size, uint32_t level, uint32_t layer) const = 0;
            virtual void SetSampler(const Structs::SamplerDescriptor& sampler) = 0;
            virtual void Import(const char* filepath, void* pParams) = 0;
            virtual bool Validate(const Math::uint3& resolution) = 0;
            virtual bool Validate(const uint32_t levels, const uint32_t layers) = 0;
            virtual bool Validate(const Structs::TextureDescriptor& descriptor) = 0;

            constexpr const Structs::SamplerDescriptor& GetSamplerDescriptor() const { return m_descriptor.sampler; }
            constexpr const Math::uint4 GetRect() const { return { 0, 0, m_descriptor.resolution.x, m_descriptor.resolution.y }; }
            constexpr const Math::uint3 GetResolution() const { return m_descriptor.resolution; }
            constexpr const uint32_t GetLevels() const { return m_descriptor.levels; }
            constexpr const uint32_t GetLayers() const { return m_descriptor.layers; }

        protected:
            Structs::TextureDescriptor m_descriptor;
    };
}