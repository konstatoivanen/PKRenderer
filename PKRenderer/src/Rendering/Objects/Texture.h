#pragma once
#include "Utilities/NativeInterface.h"
#include "Core/Services/AssetDatabase.h"
#include "Rendering/Structs/Descriptors.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::Structs;

    class Texture : public Asset, public NativeInterface<Texture>
    {
        friend Ref<Texture> AssetImporters::Create();

        public:
            static Ref<Texture> Create(const TextureDescriptor& descriptor);

            virtual ~Texture() = default;
            virtual void SetData(const void* data, size_t size, uint32_t level, uint32_t layer) const = 0;
            virtual void Import(const char* filepath) = 0;
            virtual bool Validate(const uint3 resolution) = 0;
            virtual bool Validate(const TextureDescriptor& descriptor) = 0;

            constexpr const SamplerDescriptor& GetSamplerDescriptor() const { return m_descriptor.sampler; }
            constexpr const uint4 GetRect() const { return { 0, 0, m_descriptor.resolution.x, m_descriptor.resolution.y }; }
            constexpr const uint3 GetResolution() const { return m_descriptor.resolution; }

        protected:
            TextureDescriptor m_descriptor;
    };
}