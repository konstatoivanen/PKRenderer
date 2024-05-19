#pragma once
#include "Utilities/NativeInterface.h"
#include "Graphics/RHI/Structs.h"
#include "Graphics/RHI/Layout.h"

namespace PK::Graphics::RHI
{
    struct RHIShader : public Utilities::NoCopy, public Utilities::NativeInterface<RHIShader>
    {
        virtual ~RHIShader() = 0;
        constexpr const VertexInputLayout& GetVertexLayout() const { return m_vertexLayout; }
        constexpr const PushConstantLayout& GetPushConstantLayout() const { return m_pushConstantLayout; }
        constexpr const ResourceLayout& GetResourceLayout(uint32_t set) const { return m_resourceLayouts[set]; }
        constexpr const ShaderStageFlags GetStageFlags() const { return m_stageFlags; }
        constexpr const Math::uint3& GetGroupSize() const { return m_groupSize; }
        virtual ShaderBindingTableInfo GetShaderBindingTableInfo() const = 0;
        inline bool HasRayTracingShaderGroup(RayTracingShaderGroup group) const { return (PK_RAYTRACING_GROUP_SHADER_STAGE[(uint32_t)group] & m_stageFlags) != 0; }

    protected:
        VertexInputLayout m_vertexLayout;
        PushConstantLayout m_pushConstantLayout;
        ResourceLayout m_resourceLayouts[PK_MAX_DESCRIPTOR_SETS];
        ShaderStageFlags m_stageFlags = ShaderStageFlags::None;
        Math::uint3 m_groupSize{};
    };
}