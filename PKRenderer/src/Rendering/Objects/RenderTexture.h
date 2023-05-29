#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering::Objects
{
    class RenderTexture : public Utilities::NoCopy
    {
        public:
            RenderTexture(const Structs::RenderTextureDescriptor& descriptor, const char* name);

            bool Validate(Math::uint3 resolution);

            constexpr uint32_t GetColorCount() const { return m_colorAttachmentCount; }
            inline Texture* GetColor(uint32_t index) const { return index >= m_colorAttachmentCount ? nullptr : m_colorAttachments[index].get(); }
            inline Texture* GetDepth() const { return m_depthAttachment == nullptr ? nullptr : m_depthAttachment.get(); }
            constexpr const Math::uint4 GetRect() const { return { 0, 0, m_descriptor.resolution.x, m_descriptor.resolution.y }; }
            constexpr const Math::uint3 GetResolution() const { return m_descriptor.resolution; }

        private:
            Structs::RenderTextureDescriptor m_descriptor;
            Utilities::Ref<Texture> m_colorAttachments[Structs::PK_MAX_RENDER_TARGETS]{};
            Utilities::Ref<Texture> m_depthAttachment = nullptr;
            uint32_t m_colorAttachmentCount = 0u;
    };
}