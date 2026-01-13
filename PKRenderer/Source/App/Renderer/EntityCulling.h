#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/FastBuffer.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)

namespace PK::App
{
    struct CulledEntityInfo
    {
        uint32_t entityId;
        uint16_t depth;
        uint16_t clipId;
    };

    struct CulledEntityInfoList : public NoCopy
    {
        FastBuffer<CulledEntityInfo> data;
        size_t count;
        CulledEntityInfoList(size_t reserve, size_t count) : data(reserve), count(count) {}
        void Add(uint32_t entityId, uint16_t depth, uint16_t clipId);
        inline void Clear() { count = 0ull; }
        inline const CulledEntityInfo& operator [] (size_t index) const { return data[index]; }
        inline CulledEntityInfo& GetAt(size_t index) { return data[index]; }
    };

    struct RequestEntityCullResults
    {
        ConstBufferView<CulledEntityInfo> outResults{};
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
        float4x4 matrix;
    };

    struct RequestEntityCullCubeFaces : public RequestEntityCullResults
    {
        ScenePrimitiveFlags mask;
        BoundingBox aabb;
    };

    struct RequestEntityCullCascades : public RequestEntityCullResults
    {
        ScenePrimitiveFlags mask;
        float4x4* cascades;
        float4 viewForwardPlane;
        // Needs to be valid for count + 1
        const float* viewZOffsets;
        uint32_t count;
    };

    struct RequestEntityCullRayTracingGeometry
    {
        ScenePrimitiveFlags mask;
        BoundingBox bounds;
        bool useBounds;
        QueueType queue;
        RHIAccelerationStructure* structure;
    };

    struct EntityCullSequencerProxy
    {
        Sequencer* sequencer;
        void* sequencerRoot;

        EntityCullSequencerProxy(Sequencer* sequencer, void* sequencerRoot) :
            sequencer(sequencer),
            sequencerRoot(sequencerRoot)
        {
        }

        RequestEntityCullResults CullFrustum(ScenePrimitiveFlags mask, const float4x4& matrix);
        RequestEntityCullResults CullCubeFaces(ScenePrimitiveFlags mask, const BoundingBox& aabb);
        RequestEntityCullResults CullCascades(ScenePrimitiveFlags mask, float4x4* cascades, const float4& viewForwardPlane, const float* viewZOffsets, uint32_t count);
        void CullRayTracingGeometry(ScenePrimitiveFlags mask, const BoundingBox& bounds, bool useBounds, QueueType queue, RHIAccelerationStructure* structure);
    };
}
