#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    class VulkanShader : public RHI::Objects::ShaderVariant, public PK::Utilities::VersionedObject
    {
        public:
            VulkanShader(void* base, PK::Assets::Shader::PKShaderVariant* variant, const char* name);
            ~VulkanShader();

            void Dispose() final;

            const VulkanShaderModule* GetModule(uint32_t index) const { return m_modules[index]; }
            const char* GetName() const { return m_name.c_str(); }
            constexpr const VulkanDescriptorSetLayout* GetDescriptorSetLayout(uint32_t index) const { return m_descriptorSetLayouts[index]; }
            constexpr const VulkanPipelineLayout* GetPipelineLayout() const { return m_pipelineLayout; }
            constexpr const uint32_t GetDescriptorSetCount() const { return m_descriptorSetCount; }
            ShaderBindingTableInfo GetShaderBindingTableInfo() const final;

        private:
            const VkDevice m_device;
            uint32_t m_descriptorSetCount;
            VulkanShaderModule* m_modules[(size_t)ShaderStage::MaxCount];
            const VulkanDescriptorSetLayout* m_descriptorSetLayouts[PK_MAX_DESCRIPTOR_SETS]{};
            const VulkanPipelineLayout* m_pipelineLayout;
            const std::string m_name;
    };
}