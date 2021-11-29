#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include <PKAssets/PKAssetLoader.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::Structs;

    class VulkanShader : public PK::Rendering::Objects::ShaderVariant, public VulkanDisposable
    {
        public:
            VulkanShader(void* base, PK::Assets::Shader::PKShaderVariant* variant);

            void Release() override final;

            const VulkanShaderModule* GetModule(uint32_t index) const { return m_modules[index] != nullptr ? m_modules[index].get() : nullptr; }
            const VulkanDescriptorSetLayout* GetDescriptorSetLayout(uint32_t index) const { return m_descriptorSetLayouts[index].get(); }
            const VulkanPipelineLayout* GetPipelineLayout() const { return m_pipelineLayout.get(); }

        private:
            VkDevice m_device;
            Ref<VulkanShaderModule> m_modules[(int)ShaderStage::MaxCount];
            Ref<VulkanDescriptorSetLayout> m_descriptorSetLayouts[PK_MAX_DESCRIPTOR_SETS]{};
            Ref<VulkanPipelineLayout> m_pipelineLayout;
    };
}