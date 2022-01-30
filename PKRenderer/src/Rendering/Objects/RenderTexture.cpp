#include "PrecompiledHeader.h"
#include "RenderTexture.h"

namespace PK::Rendering::Objects
{
    using namespace Structs;
    using namespace Math;

    RenderTexture::RenderTexture(const RenderTextureDescriptor& descriptor) : m_descriptor(descriptor)
    {
        m_colorAttachmentCount = 0u;

        auto usageColor = (descriptor.usage & TextureUsage::ValidRTColorUsages) | TextureUsage::RTColor;
        auto usageDepth = (descriptor.usage & TextureUsage::ValidRTDepthUsages) | TextureUsage::RTDepth;

        for (auto i = 0u; i < PK_MAX_RENDER_TARGETS; ++i)
        {
            if (descriptor.colorFormats[i] != TextureFormat::Invalid)
            {
                m_descriptor.colorFormats[m_colorAttachmentCount] = descriptor.colorFormats[i];
                
                TextureDescriptor attachmentDescriptor{};
                attachmentDescriptor.format = descriptor.colorFormats[i];
                attachmentDescriptor.samplerType = descriptor.samplerType; 
                attachmentDescriptor.usage = usageColor;
                attachmentDescriptor.resolution = descriptor.resolution;
                attachmentDescriptor.levels = descriptor.levels;
                attachmentDescriptor.samples = descriptor.samples;
                attachmentDescriptor.layers = descriptor.layers;
                attachmentDescriptor.sampler = descriptor.sampler;
                m_colorAttachments[m_colorAttachmentCount++] = Texture::Create(attachmentDescriptor);
            }
        }

        if (descriptor.depthFormat != TextureFormat::Invalid)
        {
            TextureDescriptor attachmentDescriptor{};
            attachmentDescriptor.format = descriptor.depthFormat;
            attachmentDescriptor.samplerType = descriptor.samplerType; 
            attachmentDescriptor.usage = usageDepth;
            attachmentDescriptor.resolution = descriptor.resolution;
            attachmentDescriptor.levels = descriptor.levels;
            attachmentDescriptor.samples = descriptor.samples;
            attachmentDescriptor.layers = descriptor.layers;
            attachmentDescriptor.sampler = descriptor.sampler;
            m_depthAttachment = Texture::Create(attachmentDescriptor);
        }
    }
    
    void RenderTexture::Validate(uint3 resolution)
    {
        if (m_descriptor.resolution == resolution)
        {
            return;
        }

        m_descriptor.resolution = resolution;

        for (auto i = 0u; i < m_colorAttachmentCount; ++i)
        {
            m_colorAttachments[i]->Validate(resolution);
        }

        if (m_depthAttachment != nullptr)
        {
            m_depthAttachment->Validate(resolution);
        }
    }
}