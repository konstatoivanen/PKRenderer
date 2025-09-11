#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Layout.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanShader : public RHIShader, public VersionedObject
    {
        public:
            VulkanShader(void* base, PKAssets::PKShaderVariant* variant, const char* name);
            ~VulkanShader();

            const ShaderVertexInputLayout& GetVertexLayout() const final { return m_vertexLayout; }
            const ShaderPushConstantLayout& GetPushConstantLayout() const final { return m_pushConstantLayout; }
            const ShaderResourceLayout& GetResourceLayout(uint32_t set) const final { return m_resourceLayouts[set]; }
            ShaderStageFlags GetStageFlags() const final { return m_stageFlags; }
            const uint3& GetGroupSize() const final { return m_groupSize; }
            ShaderBindingTableInfo GetShaderBindingTableInfo() const final;

            VkShaderModule GetModule(uint32_t index) const { return m_modules[index]; }
            const char* GetName() const { return m_name.c_str(); }
            constexpr const VulkanDescriptorSetLayout* GetDescriptorSetLayout(uint32_t index) const { return m_descriptorSetLayouts[index]; }
            constexpr const VulkanPipelineLayout* GetPipelineLayout() const { return m_pipelineLayout; }
            constexpr uint32_t GetDescriptorSetCount() const { return m_descriptorSetCount; }

        private:
            const VkDevice m_device;
            ShaderVertexInputLayout m_vertexLayout;
            ShaderPushConstantLayout m_pushConstantLayout;
            ShaderResourceLayout m_resourceLayouts[PK_RHI_MAX_DESCRIPTOR_SETS];
            ShaderStageFlags m_stageFlags = ShaderStageFlags::None;
            uint32_t m_descriptorSetCount;
            uint3 m_groupSize{};

            VkShaderModule m_modules[(size_t)ShaderStage::MaxCount];
            const VulkanDescriptorSetLayout* m_descriptorSetLayouts[PK_RHI_MAX_DESCRIPTOR_SETS]{};
            const VulkanPipelineLayout* m_pipelineLayout;
            const FixedString128 m_name;
    };
}
