#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Layout.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanShader : public RHIShader, public VersionedObject
    {
        public:
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
            constexpr const VulkanDescriptorSetLayout* GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
            constexpr const VulkanPipelineLayout* GetPipelineLayout() const { return m_pipelineLayout; }

        private:
            const VulkanDriver* m_driver;
            ShaderVertexInputLayout m_vertexLayout;
            ShaderPushConstantLayout m_pushConstantLayout;
            ShaderResourceLayout m_resourceLayout;
            ShaderStageFlags m_stageFlags = ShaderStageFlags::None;
            uint3 m_groupSize{};

            VkShaderModule m_modules[(size_t)ShaderStage::MaxCount];
            const VulkanDescriptorSetLayout* m_descriptorSetLayout;
            const VulkanPipelineLayout* m_pipelineLayout;
            const FixedString128 m_name;
    };
}
