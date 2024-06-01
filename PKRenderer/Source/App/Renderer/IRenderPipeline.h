#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    struct IBatcher;
    struct RenderView;
    struct GBuffersFullDescriptor;

    struct RenderPipelineContext
    {
        Sequencer* sequencer;
        EntityDatabase* entityDb;
        struct EntityCullSequencerProxy* cullingProxy;
        IBatcher* batcher;
        RHIWindow* window;
        RenderView** views;
        uint32_t viewCount;
    };

    struct RenderPipelineEvent
    {
        // Events forwarded out of renderpipeline to engines that might use them
        typedef enum
        {
            CollectDraws,
            Depth,
            GBuffer,
            ForwardOpaque,
            ForwardTransparent,
            AfterPostEffects,
            Count
        } Type;

        constexpr static const char* TypeNames[Count] =
        {
            "CollectDraws",
            "Depth",
            "GBuffer",
            "ForwardOpaque",
            "ForwardTransparent",
            "AfterPostEffects"
        };

        Type type;
        CommandBufferExt cmd;
        RenderPipelineContext* context;
    };

    struct IRenderPipeline
    {
        virtual ~IRenderPipeline() = 0;
        virtual GBuffersFullDescriptor GetViewGBufferDescriptors() const = 0;
        virtual const BufferLayout& GetViewConstantsLayout() const = 0;
        virtual void SetViewConstants(RenderView* view) = 0;
        virtual void RenderViews(RenderPipelineContext* context) = 0;
        protected: void DispatchRenderPipelineEvent(RHICommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type);
    };
}
