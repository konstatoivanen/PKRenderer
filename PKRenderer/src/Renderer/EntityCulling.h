#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Utilities/MemoryBlock.h"
#include "Math/Types.h"
#include "Graphics/GraphicsFwd.h"
#include "Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)

namespace PK::Renderer
{
    struct CulledEntityInfo
    {
        uint32_t entityId;
        uint16_t depth;
        uint16_t clipId;
    };

    struct CulledEntityInfoList : public Utilities::NoCopy
    {
        Utilities::MemoryBlock<CulledEntityInfo> data;
        size_t count;
        CulledEntityInfoList(size_t reserve, size_t count) : data(reserve), count(count) {}
        inline void Add(uint32_t entityId, uint16_t depth, uint16_t clipId) { data.SetValidate(count++, { entityId, depth, clipId }); }
        inline void Clear() { count = 0ull; }
        inline const CulledEntityInfo& operator [] (size_t index) const { return data[index]; }
        inline CulledEntityInfo& GetAt(size_t index) { return data[index]; }
    };

    struct RequestEntityCullResults
    {
        Utilities::ConstBufferView<CulledEntityInfo> outResults{};
        float outMinDepth = 0.0f;
        float outMaxDepth = 0.0f;
        float outDepthRange = 0.0f;

        const CulledEntityInfo& operator[](size_t i) const
        {
            return outResults.data[i];
        }

        size_t GetCount() const { return outResults.count; }
    };

    struct RequestEntityCullFrustum : public RequestEntityCullResults
    {
        ScenePrimitiveFlags mask;
        Math::float4x4 matrix;
    };

    struct RequestEntityCullCubeFaces : public RequestEntityCullResults
    {
        ScenePrimitiveFlags mask;
        Math::BoundingBox aabb;
    };

    struct RequestEntityCullCascades : public RequestEntityCullResults
    {
        ScenePrimitiveFlags mask;
        Math::float4x4* cascades;
        Math::float4 viewForwardPlane;
        // Needs to be valid for count + 1
        const float* viewZOffsets;
        uint32_t count;
    };

    struct RequestEntityCullRayTracingGeometry
    {
        Renderer::ScenePrimitiveFlags mask;
        Math::BoundingBox bounds;
        bool useBounds;
        Graphics::RHI::QueueType queue;
        Graphics::AccelerationStructure* structure;
    };

    struct EntityCullSequencerProxy
    {
        PK::Core::ControlFlow::Sequencer* sequencer;
        void* sequencerRoot;

        EntityCullSequencerProxy(PK::Core::ControlFlow::Sequencer* sequencer, void* sequencerRoot) :
            sequencer(sequencer),
            sequencerRoot(sequencerRoot)
        {
        }

        RequestEntityCullResults CullFrustum(ScenePrimitiveFlags mask, const Math::float4x4& matrix);
        RequestEntityCullResults CullCubeFaces(ScenePrimitiveFlags mask, const Math::BoundingBox& aabb);
        RequestEntityCullResults CullCascades(ScenePrimitiveFlags mask, Math::float4x4* cascades, const Math::float4& viewForwardPlane, const float* viewZOffsets, uint32_t count);
        void CullRayTracingGeometry(ScenePrimitiveFlags mask, const Math::BoundingBox& bounds, bool useBounds, Graphics::RHI::QueueType queue, Graphics::AccelerationStructure* structure);
    };
}
