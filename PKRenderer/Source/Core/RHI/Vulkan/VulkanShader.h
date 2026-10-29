#pragma once
#include "Core/Base/Types/VersionedObject.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Layout.h"

namespace PK
{
    struct VulkanShader : public RHIShader, public VersionedObject
    {
        VulkanShader(struct VulkanDriver* driver, void* base, PKAssets::PKShaderVariant* variant, const char* name);
        ~VulkanShader();

        const ShaderVertexInputLayout& GetVertexLayout() const final { return m_vertexLayout; }
        const ShaderPushConstantLayout& GetPushConstantLayout() const final { return m_pushConstantLayout; }
        ShaderStageFlags GetStageFlags() const final { return m_stageFlags; }
        const uint3& GetGroupSize() const final { return m_groupSize; }
        ShaderBindingTableInfo GetShaderBindingTableInfo() const final;

        const VkShaderEXT* GetShaderObjects() const { return m_shaders; }
        VkPipeline GetRayTracingPipeline() const { return m_rayTracingPipeline; }
        VkPipelineBindPoint GetBindPoint() const { return m_bindPoint; }
        const char* GetName() const { return m_name.c_str(); }
        constexpr uint32_t GetPushConstantSize() const { return (uint32_t)m_pushConstantLayout.sizeBytes; }

    private:
        const VulkanDriver* m_driver;
        const FixedString128 m_name;
        ShaderVertexInputLayout m_vertexLayout;
        ShaderPushConstantLayout m_pushConstantLayout;
        ShaderStageFlags m_stageFlags = ShaderStageFlags::None;
        uint3 m_groupSize{};

        VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        VkShaderEXT m_shaders[(uint32_t)ShaderStage::EnumCount]{};
        VkPipeline m_rayTracingPipeline = VK_NULL_HANDLE;
    };
}