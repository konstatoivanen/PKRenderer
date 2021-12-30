#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;
    using namespace Structs;

    class RenderTexture : public NoCopy
    {
        public:
            RenderTexture(const RenderTextureDescriptor& descriptor);

            void Validate(uint3 resolution);

            constexpr uint GetColorCount() const { return m_colorAttachmentCount; }
            inline Texture* GetColor(uint index) const { return index >= m_colorAttachmentCount ? nullptr : m_colorAttachments[index].get(); }
            inline Texture* GetDepth() const { return m_depthAttachment == nullptr ? nullptr : m_depthAttachment.get(); }
            constexpr const uint4 GetRect() const { return { 0, 0, m_descriptor.resolution.x, m_descriptor.resolution.y }; }
            constexpr const uint3 GetResolution() const { return m_descriptor.resolution; }

        private:
            RenderTextureDescriptor m_descriptor;
            Ref<Texture> m_colorAttachments[PK_MAX_RENDER_TARGETS]{};
            Ref<Texture> m_depthAttachment = nullptr;
            uint m_colorAttachmentCount = 0u;
    };
}