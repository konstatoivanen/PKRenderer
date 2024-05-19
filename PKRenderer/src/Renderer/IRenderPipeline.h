#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/IService.h"
#include "Graphics/CommandBufferExt.h"
#include "Graphics/GraphicsFwd.h"
#include "Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct IBatcher)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct RenderView)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct GBuffersFullDescriptor)

namespace PK::Renderer
{
    struct RenderPipelineContext
    {
        PK::Core::ControlFlow::Sequencer* sequencer;
        PK::ECS::EntityDatabase* entityDb;
        struct EntityCullSequencerProxy* cullingProxy;
        IBatcher* batcher;
        Graphics::Window* window;
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
        Graphics::CommandBufferExt cmd;
        RenderPipelineContext* context;
    };

    struct IRenderPipeline : public Core::IService
    {
        virtual GBuffersFullDescriptor GetViewGBufferDescriptors() const = 0;
        virtual const Graphics::RHI::BufferLayout& GetViewConstantsLayout() const = 0;
        virtual void SetViewConstants(RenderView* view) = 0;
        virtual void RenderViews(RenderPipelineContext* context) = 0;
        protected: void DispatchRenderPipelineEvent(Graphics::CommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type);
    };
}
