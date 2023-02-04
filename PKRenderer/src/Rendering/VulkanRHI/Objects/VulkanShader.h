#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include <PKAssets/PKAssetLoader.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanShader : public PK::Rendering::Objects::ShaderVariant, public Rendering::Services::IDisposable
    {
        public:
            VulkanShader(void* base, PK::Assets::Shader::PKShaderVariant* variant, const char* name);
            ~VulkanShader();

            void Dispose() override final;

            const VulkanShaderModule* GetModule(uint32_t index) const { return m_modules[index]; }
            const char* GetName() const { return m_name.c_str(); }
            constexpr const VulkanDescriptorSetLayout* GetDescriptorSetLayout(uint32_t index) const { return m_descriptorSetLayouts[index]; }
            constexpr const VulkanPipelineLayout* GetPipelineLayout() const { return m_pipelineLayout; }
            constexpr const uint32_t GetDescriptorSetCount() const { return m_descriptorSetCount; }
            Structs::ShaderBindingTableInfo GetShaderBindingTableInfo() const override final;

        private:
            const VkDevice m_device;
            uint32_t m_descriptorSetCount;
            VulkanShaderModule* m_modules[(int)Structs::ShaderStage::MaxCount];
            const VulkanDescriptorSetLayout* m_descriptorSetLayouts[Structs::PK_MAX_DESCRIPTOR_SETS]{};
            const VulkanPipelineLayout* m_pipelineLayout;
            const std::string m_name;
    };
}