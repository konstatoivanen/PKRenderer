#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/Layout.h"
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::Structs;

    class VulkanShader : public PK::Core::NoCopy
    {
        public:
            VulkanShader(VkDevice device, const VulkanShaderCreateInfo& createInfo);

            const VulkanShaderModule* GetModule(uint32_t index) const { return m_modules[index] != nullptr ? m_modules[index].get() : nullptr; }

            constexpr const VertexLayout& GetVertexLayout() const { return m_vertexLayout; }

            constexpr const ResourceLayout& GetResourceLayout(uint32_t index) const { return m_resourceLayouts[index]; }

            const VulkanDescriptorSetLayout* GetDescriptorSetLayout(uint32_t index) const { return m_descriptorSetLayouts[index].get(); }

            const VulkanPipelineLayout* GetPipelineLayout() const { return m_pipelineLayout.get(); }

            constexpr const ShaderType GetShaderType() const { return m_type; }

        private:
            Ref<VulkanShaderModule> m_modules[(int)ShaderStage::MaxCount];
            VertexLayout m_vertexLayout;
            ResourceLayout m_resourceLayouts[PK_MAX_DESCRIPTOR_SETS];
            Ref<VulkanDescriptorSetLayout> m_descriptorSetLayouts[PK_MAX_DESCRIPTOR_SETS]{};
            Ref<VulkanPipelineLayout> m_pipelineLayout;
            ShaderType m_type;
    };
}