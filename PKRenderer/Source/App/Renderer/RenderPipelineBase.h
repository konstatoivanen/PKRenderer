#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/EntityEnums.h"
#include "App/Renderer/IBatcher.h"
#include "App/Renderer/RenderView.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

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

    struct RenderPipelineBase : 
        public IStepApplicationRenderWindow,
        public IStep<TimeFrameInfo*>
    {
        constexpr static size_t MAX_RENDER_VIEWS = 32ull;

        RenderPipelineBase(EntityDatabase* entityDb,
            AssetDatabase* assetDatabase,
            Sequencer* sequencer,
            IBatcher* batcher);

        void Step(TimeFrameInfo* token) final { m_timeFrameInfo = *token; }
        void OnApplicationRender(RHIWindow* window) final;

        protected: 
            virtual void Render(RenderPipelineContext* context) = 0;

            void ValidateViewGBuffers(RenderView* view, const GBuffersFullDescriptor& descriptors);
            void ValidateViewConstantBuffer(RenderView* view, const BufferLayout& descriptors);
            void DispatchRenderPipelineEvent(RHICommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type);
    
        private:
            Sequencer* m_sequencer = nullptr;
            EntityDatabase* m_entityDb = nullptr;
            IBatcher* m_batcher;
            RenderView m_renderViews[MAX_RENDER_VIEWS]{};
            uint32_t m_renderViewCount;

            TimeFrameInfo m_timeFrameInfo{};
    };
}
