#include "PrecompiledHeader.h"
#include "VulkanTexture.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "KTX/ktx.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;
    using namespace Systems;
    using namespace Objects;

    VulkanTexture::VulkanTexture() : m_driver(GraphicsAPI::GetActiveDriver<VulkanDriver>())
    {
    }

    VulkanTexture::VulkanTexture(const TextureDescriptor& descriptor, const char* name) : m_driver(GraphicsAPI::GetActiveDriver<VulkanDriver>()), m_name(name)
    {
        Rebuild(descriptor);
    }

    VulkanTexture::~VulkanTexture()
    {
        Dispose();
    }


	void VulkanTexture::SetData(const void* data, size_t size, uint32_t level, uint32_t layer) const
    {
        auto cmd = m_driver->commandBufferPool->GetCurrent();
        
        const auto* stage = m_driver->stagingBufferCache->GetBuffer(size, cmd->GetOnCompleteGate());
        stage->SetData(data, size);
     
        auto usageLayout = EnumConvert::GetImageLayout(m_descriptor.usage);
        auto optimalLayout = EnumConvert::GetImageLayout(m_descriptor.usage, true);
        VkImageSubresourceRange range = { (uint32_t)m_rawImage->aspect, level, 1, layer, 1 };

        cmd->TransitionImageLayout(VulkanLayoutTransition(m_rawImage->image, usageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range));
        cmd->CopyBufferToImage(stage->buffer, m_rawImage->image, { m_rawImage->extent.width >> level, m_rawImage->extent.height >> level, m_rawImage->extent.depth >> level }, level, layer);
        cmd->TransitionImageLayout(VulkanLayoutTransition(m_rawImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, usageLayout, optimalLayout, range));
    }

    void VulkanTexture::SetSampler(const SamplerDescriptor& sampler)
    {
        if (m_descriptor.sampler == sampler)
        {
            return;
        }

        m_descriptor.sampler = sampler;

        for (auto& kv : m_imageViews)
        {
            if (kv.second->bindHandle.sampler != VK_NULL_HANDLE)
            {
                kv.second->bindHandle.IncrementVersion();
                kv.second->bindHandle.sampler = GraphicsAPI::GetActiveDriver<VulkanDriver>()->samplerCache->GetSampler(m_descriptor.sampler);
            }
        }
    }

    void VulkanTexture::Import(const char* filepath, void* pParams)
    {
        Dispose();

        m_name = GetFileName();

		ktxTexture2* ktxTex2;

		TextureDescriptor descriptor{};

		auto result = ktxTexture2_CreateFromNamedFile(filepath, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex2);

        if (result != KTX_SUCCESS)
        {
            PK_THROW_ERROR(ktxErrorString(result));
        }

		descriptor.resolution = { ktxTex2->baseWidth, ktxTex2->baseHeight, ktxTex2->baseDepth };
		descriptor.levels = ktxTex2->numLevels;
		descriptor.layers = ktxTex2->numLayers;
		descriptor.format = EnumConvert::GetTextureFormat((VkFormat)ktxTex2->vkFormat);

		if (ktxTex2->isCubemap && ktxTex2->isArray)
		{
			descriptor.samplerType = SamplerType::CubemapArray;
		}
        else if (ktxTex2->isCubemap)
        {
			descriptor.samplerType = SamplerType::Cubemap;
        }
		else if (ktxTex2->isArray)
		{
			descriptor.samplerType = SamplerType::Sampler2DArray;
		}
		else if (ktxTex2->baseDepth > 1)
		{
			descriptor.samplerType = SamplerType::Sampler3D;
		}
			 
		descriptor.sampler.anisotropy = 16.0f;
		descriptor.sampler.mipMin = 0.0f;
		descriptor.sampler.mipMax = (float)ktxTex2->numLevels;
		descriptor.sampler.filterMin = ktxTex2->numLevels > 1 ? FilterMode::Trilinear : FilterMode::Bilinear;
		descriptor.sampler.filterMag = ktxTex2->numLevels > 1 ? FilterMode::Trilinear : FilterMode::Bilinear;
		descriptor.sampler.wrap[0] = WrapMode::Repeat;
		descriptor.sampler.wrap[1] = WrapMode::Repeat;
		descriptor.sampler.wrap[2] = WrapMode::Repeat;

		Rebuild(descriptor);

		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture(ktxTex2));
		ktx_size_t ktxTextureSize = ktxTex2->dataSize;

        auto cmd = m_driver->commandBufferPool->GetCurrent();
		auto stage = m_driver->stagingBufferCache->GetBuffer(ktxTextureSize, cmd->GetOnCompleteGate());
        
		stage->SetData(ktxTextureData, ktxTextureSize);

		std::vector<VkBufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;

        auto slices = ktxTex2->isCubemap ? ktxTex2->numFaces : descriptor.resolution.z;

        for (auto layer = 0u; layer < descriptor.layers; ++layer)
		for (auto level = 0u; level < descriptor.levels; ++level) 
        for (auto slice = 0u; slice < slices; ++slice)
		{
			ktx_size_t offset;
			PK_THROW_ASSERT(ktxTexture_GetImageOffset(ktxTexture(ktxTex2), level, layer, slice, &offset) == KTX_SUCCESS, "Failed to get image buffer offset");

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = m_rawImage->aspect;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = ktxTex2->isCubemap ? ((layer * slices) + slice) : layer;
			bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = descriptor.resolution.x > 1 ? descriptor.resolution.x >> level : 1;
			bufferCopyRegion.imageExtent.height = descriptor.resolution.y > 1 ? descriptor.resolution.y >> level : 1;
			bufferCopyRegion.imageExtent.depth = descriptor.resolution.z > 1 ? descriptor.resolution.z >> level : 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}

		auto usageLayout = EnumConvert::GetImageLayout(m_descriptor.usage);
		auto optimalLayout = EnumConvert::GetImageLayout(m_descriptor.usage, true);
		VkImageSubresourceRange range = { (uint32_t)m_rawImage->aspect, 0, descriptor.levels, 0, descriptor.layers };

		cmd->TransitionImageLayout(VulkanLayoutTransition(m_rawImage->image, usageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range));
        cmd->CopyBufferToImage(stage->buffer, m_rawImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)bufferCopyRegions.size(), bufferCopyRegions.data());
		cmd->TransitionImageLayout(VulkanLayoutTransition(m_rawImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, usageLayout, optimalLayout, range));

        ktxTexture_Destroy(ktxTexture(ktxTex2));
    }


    bool VulkanTexture::Validate(const uint3& resolution)
    {
        if (m_descriptor.resolution == resolution)
        {
            return false;
        }

        auto descriptor = m_descriptor;
        descriptor.resolution = resolution;
        Rebuild(descriptor);
        return true;
    }

    bool VulkanTexture::Validate(const uint32_t levels, const uint32_t layers)
    {
        if (m_descriptor.levels == levels && m_descriptor.layers == layers)
        {
            return false;
        }

        auto descriptor = m_descriptor;
        descriptor.levels = levels;
        descriptor.layers = layers;
        Rebuild(descriptor);
        return true;
    }

    bool VulkanTexture::Validate(const TextureDescriptor& descriptor)
    {
        if (m_descriptor.samplerType == descriptor.samplerType &&
            m_descriptor.format == descriptor.format &&
            m_descriptor.usage == descriptor.usage &&
            m_descriptor.resolution == descriptor.resolution &&
            m_descriptor.levels == descriptor.levels &&
            m_descriptor.samples == descriptor.samples &&
            m_descriptor.layers == descriptor.layers)
        {
            return false;
        }

        Rebuild(descriptor);
        return true;
    }

    void VulkanTexture::Rebuild(const TextureDescriptor& descriptor)
    {
        Dispose();

        m_descriptor = descriptor;
        m_rawImage = new VulkanRawImage(m_driver->device, m_driver->allocator, VulkanImageCreateInfo(descriptor), m_name.c_str());

        m_viewType = EnumConvert::GetViewType(descriptor.samplerType);
        m_swizzle = EnumConvert::GetSwizzle(m_rawImage->format);

        m_defaultViewRange.level = 0;
        m_defaultViewRange.layer = 0;
        m_defaultViewRange.levels = m_rawImage->levels;
        m_defaultViewRange.layers = m_rawImage->layers;

        GetView(m_defaultViewRange);

        if ((descriptor.usage & TextureUsage::ValidRTTypes) != 0)
        {
            GetView(m_defaultViewRange, TextureBindMode::RenderTarget);
        }

        auto layout = EnumConvert::GetImageLayout(descriptor.usage);
        auto optimalLayout = EnumConvert::GetImageLayout(descriptor.usage, true);
        VulkanLayoutTransition transition(m_rawImage->image, VK_IMAGE_LAYOUT_UNDEFINED, layout, optimalLayout, { (uint32_t)m_rawImage->aspect, 0, m_rawImage->levels, 0, m_defaultViewRange.layers });
        m_driver->commandBufferPool->GetCurrent()->TransitionImageLayout(transition);
    }

    TextureViewRange VulkanTexture::NormalizeViewRange(const TextureViewRange& range) const
    {
        auto out = range;

        switch (m_viewType)
        {
            case VK_IMAGE_VIEW_TYPE_1D:
            case VK_IMAGE_VIEW_TYPE_2D:
            case VK_IMAGE_VIEW_TYPE_3D: out.layers = 1u; break;
            case VK_IMAGE_VIEW_TYPE_CUBE: out.layers = 6u; break;
        }

        if (out.level >= m_defaultViewRange.levels)
        {
            out.level = m_defaultViewRange.levels - 1u;
        }

        if (out.layer >= m_defaultViewRange.layers)
        {
            out.layer = m_defaultViewRange.layers - 1u;
        }

        if (out.levels == 0)
        {
            out.levels = m_defaultViewRange.levels;
        }
        else if (out.level + out.levels > m_defaultViewRange.levels)
        {
            out.levels = m_defaultViewRange.levels - out.level;
        }

        if (out.layers == 0)
        {
            out.layers = m_defaultViewRange.layers;
        }
        else if (out.layer + out.layers > m_defaultViewRange.layers)
        {
            out.layers = m_defaultViewRange.layers - out.layer;
        }

        return out;
    }


    VulkanRenderTarget VulkanTexture::GetRenderTarget() const
    {
        auto view = GetView(m_defaultViewRange, TextureBindMode::RenderTarget)->view;

        return VulkanRenderTarget
        (
            view->view,
            m_rawImage->image,
            GetImageLayout(),
            m_rawImage->aspect,
            m_rawImage->format,
            m_rawImage->extent,
            m_descriptor.samples,
            m_descriptor.layers
        );
    }

    VulkanRenderTarget VulkanTexture::GetRenderTarget(const TextureViewRange& range, bool includeView)
    {
        auto normalizedRange = NormalizeViewRange(range);

        VkImageView view = VK_NULL_HANDLE;
        
        if (includeView)
        {
            view = GetView(normalizedRange, TextureBindMode::RenderTarget)->view->view;
        }

        return VulkanRenderTarget
        (
            view,
            m_rawImage->image,
            GetImageLayout(),
            m_rawImage->aspect,
            m_rawImage->format,
            m_rawImage->extent,
            m_descriptor.samples,
            normalizedRange.layers
        );
    }


    const VulkanTexture::ViewValue* VulkanTexture::GetView(const TextureViewRange& range, TextureBindMode mode)
	{
        ViewKey key = { NormalizeViewRange(range), mode };
        auto iter = m_imageViews.find(key);
        
        if (iter != m_imageViews.end())
        {
            return iter->second.get();
        }

        VkImageViewCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        info.pNext = nullptr;
        info.flags = 0;
        info.image = m_rawImage->image;
        info.viewType = m_viewType;
        info.format = m_rawImage->format;
        info.components = mode == TextureBindMode::SampledTexture ? m_swizzle : (VkComponentMapping{});
        info.subresourceRange = 
        {
            (uint32_t)m_rawImage->aspect,
            key.range.level,
            key.range.levels,
            key.range.layer,
            key.range.layers
        };

        auto viewValue = new VulkanTexture::ViewValue();
        viewValue->view = new VulkanImageView(m_driver->device, info, m_name.c_str());
        viewValue->bindHandle.imageLayout = GetImageLayout();
        viewValue->bindHandle.imageView = viewValue->view->view;

        if (mode == TextureBindMode::SampledTexture)
        {
            viewValue->bindHandle.sampler = GraphicsAPI::GetActiveDriver<VulkanDriver>()->samplerCache->GetSampler(m_descriptor.sampler);
        }

        m_imageViews[key] = Scope<VulkanTexture::ViewValue>(viewValue);
        return viewValue;
	}

    void VulkanTexture::Dispose()
    {
        auto gate = m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate();

        for (auto& kv : m_imageViews)
        {
            m_driver->disposer->Dispose(kv.second->view, gate);
        }

        m_imageViews.clear();

        if (m_rawImage != nullptr)
        {
            m_driver->disposer->Dispose(m_rawImage, gate);
            m_rawImage = nullptr;
        }
    }
}