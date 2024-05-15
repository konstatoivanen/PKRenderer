#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/IService.h"
#include "Rendering/EntityEnums.h"
#include "Rendering/RHI/RHI.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Geometry, struct IBatcher)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct RenderView)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct GBuffersFullDescriptor)

namespace PK::Rendering
{
    struct RenderPipelineContext
    {
        PK::Core::ControlFlow::Sequencer* sequencer;
        PK::ECS::EntityDatabase* entityDb;
        struct EntityCullSequencerProxy* cullingProxy;
        Geometry::IBatcher* batcher;
        RHI::Objects::Window* window;
        Rendering::Objects::RenderView** views;
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
        Rendering::RHI::Objects::CommandBuffer* cmd;
        RenderPipelineContext* context;
    };

    struct IRenderPipeline : public Core::IService
    {
        virtual Rendering::Objects::GBuffersFullDescriptor GetViewGBufferDescriptors() const = 0;
        virtual const RHI::BufferLayout& GetViewConstantsLayout() const = 0;
        virtual void SetViewConstants(Rendering::Objects::RenderView* view) = 0;
        virtual void RenderViews(RenderPipelineContext* context) = 0;
        protected: void DispatchRenderPipelineEvent(RHI::Objects::CommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type);
    };
}
