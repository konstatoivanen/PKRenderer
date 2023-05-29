#include "PrecompiledHeader.h"
#include "RenderTexture.h"

namespace PK::Rendering::Objects
{
    using namespace Structs;
    using namespace Math;

    RenderTexture::RenderTexture(const RenderTextureDescriptor& descriptor, const char* name) : m_descriptor(descriptor)
    {
        m_colorAttachmentCount = 0u;

        auto usageColor = (descriptor.usage & TextureUsage::ValidRTColorUsages) | TextureUsage::RTColor;
        auto usageDepth = (descriptor.usage & TextureUsage::ValidRTDepthUsages) | TextureUsage::RTDepth;

        for (auto i = 0u; i < PK_MAX_RENDER_TARGETS; ++i)
        {
            if (descriptor.colorFormats[i] != TextureFormat::Invalid)
            {
                m_descriptor.colorFormats[m_colorAttachmentCount] = descriptor.colorFormats[i];
                auto attachmentName = std::string(name) + std::string(".Color") + std::to_string(i);

                TextureDescriptor attachmentDescriptor{};
                attachmentDescriptor.format = descriptor.colorFormats[i];
                attachmentDescriptor.samplerType = descriptor.samplerType;
                attachmentDescriptor.usage = usageColor;
                attachmentDescriptor.resolution = descriptor.resolution;
                attachmentDescriptor.levels = descriptor.levels;
                attachmentDescriptor.samples = descriptor.samples;
                attachmentDescriptor.layers = descriptor.layers;
                attachmentDescriptor.sampler = descriptor.sampler;
                m_colorAttachments[m_colorAttachmentCount++] = Texture::Create(attachmentDescriptor, attachmentName.c_str());
            }
        }

        if (descriptor.depthFormat != TextureFormat::Invalid)
        {
            auto attachmentName = std::string(name) + std::string(".Depth");
            TextureDescriptor attachmentDescriptor{};
            attachmentDescriptor.format = descriptor.depthFormat;
            attachmentDescriptor.samplerType = descriptor.samplerType;
            attachmentDescriptor.usage = usageDepth;
            attachmentDescriptor.resolution = descriptor.resolution;
            attachmentDescriptor.levels = descriptor.levels;
            attachmentDescriptor.samples = descriptor.samples;
            attachmentDescriptor.layers = descriptor.layers;
            attachmentDescriptor.sampler = descriptor.sampler;
            m_depthAttachment = Texture::Create(attachmentDescriptor, attachmentName.c_str());
        }
    }

    bool RenderTexture::Validate(uint3 resolution)
    {
        if (m_descriptor.resolution == resolution)
        {
            return false;
        }

        m_descriptor.resolution = resolution;
        bool resized = false;

        for (auto i = 0u; i < m_colorAttachmentCount; ++i)
        {
            resized |= m_colorAttachments[i]->Validate(resolution);
        }

        if (m_depthAttachment != nullptr)
        {
            resized |= m_depthAttachment->Validate(resolution);
        }

        return resized;
    }
}