#pragma once
#include "Core/Base/Types/VersionedObject.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Layout.h"

namespace PK
{
struct VulkanShader : public RHIShader, public VersionedObject
    {
        struct DescriptorOffset
        {
            uint32_t offset = 0u;
            uint32_t stride = 0u;
        };

        VulkanShader(struct VulkanDriver* driver, void* base, PKAssets::PKShaderVariant* variant, const char* name);
        ~VulkanShader();

        const ShaderVertexInputLayout& GetVertexLayout() const final { return m_vertexLayout; }
        const ShaderPushConstantLayout& GetPushConstantLayout() const final { return m_pushConstantLayout; }
        const ShaderResourceLayout& GetResourceLayout() const final { return m_resourceLayout; }
        ShaderStageFlags GetStageFlags() const final { return m_stageFlags; }
        const uint3& GetGroupSize() const final { return m_groupSize; }
        ShaderBindingTableInfo GetShaderBindingTableInfo() const final;

        const VkShaderEXT* GetShaderObjects() const { return m_shaders; }
        VkPipeline GetRayTracingPipeline() const { return m_rayTracingPipeline; }
        VkPipelineBindPoint GetBindPoint() const { return m_bindPoint; }
        const char* GetName() const { return m_name.c_str(); }
        constexpr const VkShaderDescriptorSetAndBindingMappingInfoEXT* GetDescriptorLayout() const { return &m_descriptorLayout; }
        constexpr const DescriptorOffset* GetDescriptorOffsets() const { return m_descriptorOffsets; }
        constexpr uint32_t GetResourceSetSize() const { return m_resourceSetSize; }
        constexpr uint32_t GetPushConstantSize() const { return m_pushConstantSize; }

    private:
        const VulkanDriver* m_driver;
        ShaderVertexInputLayout m_vertexLayout;
        ShaderPushConstantLayout m_pushConstantLayout;
        ShaderResourceLayout m_resourceLayout;
        ShaderStageFlags m_stageFlags = ShaderStageFlags::None;
        uint32_t m_pushConstantSize = 0u;
        uint32_t m_resourceSetSize = 0u;
        uint3 m_groupSize{};

        VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        VkShaderEXT m_shaders[(uint32_t)ShaderStage::EnumCount]{};
        VkPipeline m_rayTracingPipeline = VK_NULL_HANDLE;
        VkDescriptorSetAndBindingMappingEXT m_descriptors[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        DescriptorOffset m_descriptorOffsets[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        VkShaderDescriptorSetAndBindingMappingInfoEXT m_descriptorLayout{ VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT };
        const FixedString128 m_name;
    };
}