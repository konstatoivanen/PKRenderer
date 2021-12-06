#pragma once
#include "Core/AssetDatabase.h"
#include "Core/NativeInterface.h"
#include "Rendering/Structs/Descriptors.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;
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

            const SamplerDescriptor& GetSamplerDescriptor() const { return m_descriptor.sampler; }

        protected:
            TextureDescriptor m_descriptor;
    };
}