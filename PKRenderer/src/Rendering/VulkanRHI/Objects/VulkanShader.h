#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include <PKAssets/PKAssetLoader.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::Structs;

    class VulkanShader : public PK::Rendering::Objects::ShaderVariant, public IVulkanDisposable
    {
        public:
            VulkanShader(void* base, PK::Assets::Shader::PKShaderVariant* variant);
            ~VulkanShader();

            void Dispose() override final;

            const VulkanShaderModule* GetModule(uint32_t index) const { return m_modules[index] != nullptr ? m_modules[index] : nullptr; }
            constexpr const VulkanDescriptorSetLayout* GetDescriptorSetLayout(uint32_t index) const { return m_descriptorSetLayouts[index]; }
            constexpr const VulkanPipelineLayout* GetPipelineLayout() const { return m_pipelineLayout; }
            constexpr const uint32_t GetDescriptorSetCount() const { return m_descriptorSetCount; }

        private:
            const VkDevice m_device;
            uint32_t m_descriptorSetCount;
            VulkanShaderModule* m_modules[(int)ShaderStage::MaxCount];
            VulkanDescriptorSetLayout* m_descriptorSetLayouts[PK_MAX_DESCRIPTOR_SETS]{};
            VulkanPipelineLayout* m_pipelineLayout;
    };
}