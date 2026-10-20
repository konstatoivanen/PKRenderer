#pragma once
#include "Core/Base/Types/VersionedObject.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Layout.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanShader : public RHIShader, public VersionedObject
    {
        struct DescriptorOffsets
        {
            uint32_t resourceOffset = 0u;
            uint32_t samplerOffset = 0u;
            uint16_t resourceStride = 0u;
            uint16_t samplerStride = 0u;
        };

        VulkanShader(struct VulkanDriver* driver, void* base, PKAssets::PKShaderVariant* variant, const char* name);
        ~VulkanShader();

        const ShaderVertexInputLayout& GetVertexLayout() const final { return m_vertexLayout; }
        const ShaderPushConstantLayout& GetPushConstantLayout() const final { return m_pushConstantLayout; }
        const ShaderResourceLayout& GetResourceLayout() const final { return m_resourceLayout; }
        ShaderStageFlags GetStageFlags() const final { return m_stageFlags; }
        const uint3& GetGroupSize() const final { return m_groupSize; }
        ShaderBindingTableInfo GetShaderBindingTableInfo() const final;

        VkShaderModule GetModule(uint32_t index) const { return m_modules[index]; }
        const char* GetName() const { return m_name.c_str(); }
        constexpr const VkShaderDescriptorSetAndBindingMappingInfoEXT* GetDescriptorLayout() const { return &m_descriptorLayout; }
        constexpr const DescriptorOffsets* GetDescriptorOffsets() const { return m_descriptorOffsets; }
        constexpr uint32_t GetResourceSetSize() const { return m_resourceSetSize; }
        constexpr uint32_t GetSamplerSetSize() const { return m_samplerSetSize; }
        constexpr uint32_t GetPushConstantSize() const { return m_pushConstantSize; }

    private:
        const VulkanDriver* m_driver;
        ShaderVertexInputLayout m_vertexLayout;
        ShaderPushConstantLayout m_pushConstantLayout;
        ShaderResourceLayout m_resourceLayout;
        ShaderStageFlags m_stageFlags = ShaderStageFlags::None;
        uint32_t m_pushConstantSize = 0u;
        uint32_t m_resourceSetSize = 0u;
        uint32_t m_samplerSetSize = 0u;
        uint3 m_groupSize{};

        VkShaderModule m_modules[(uint32_t)ShaderStage::EnumCount]{};
        VkDescriptorSetAndBindingMappingEXT m_descriptors[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        DescriptorOffsets m_descriptorOffsets[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        VkShaderDescriptorSetAndBindingMappingInfoEXT m_descriptorLayout{ VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT };
        const FixedString128 m_name;
    };
}