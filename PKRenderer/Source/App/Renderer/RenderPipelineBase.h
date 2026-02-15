#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/EntityEnums.h"
#include "App/Renderer/IBatcher.h"
#include "App/Renderer/RenderView.h"
#include "App/FrameStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct IArena)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

namespace PK::App
{
    struct IBatcher;
    struct RenderView;
    struct GBuffersFullDescriptor;

    struct RenderPipelineContext
    {
        IArena* frameArena;
        Sequencer* sequencer;
        EntityDatabase* entityDb;
        struct EntityCullSequencerProxy* cullingProxy;
        IBatcher* batcher;
        Window* window;
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

    struct RenderPipelineBase : public IStepFrameRender<>
    {
        constexpr static size_t MAX_RENDER_VIEWS = 32ull;

        RenderPipelineBase(EntityDatabase* entityDb,
            AssetDatabase* assetDatabase,
            Sequencer* sequencer,
            IBatcher* batcher);

        void OnStepFrameRender(FrameContext* ctx) final;

        protected: 
            virtual IRenderViewResources* GetViewResources(uint32_t index) = 0;
            virtual void Render(RenderPipelineContext* context) = 0;

            void ValidateViewGBuffers(RenderView* view, const GBuffersFullDescriptor& descriptors);
            void ValidateViewConstantBuffer(RenderView* view, const ShaderStructLayout& layout);
            void DispatchRenderPipelineEvent(RHICommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type);
    
        private:
            Sequencer* m_sequencer = nullptr;
            EntityDatabase* m_entityDb = nullptr;
            IBatcher* m_batcher;
            RenderView m_renderViews[MAX_RENDER_VIEWS]{};
            uint32_t m_renderViewCount;
            RHITextureRef m_integratedDFG;
    };

    template<typename TResources>
    struct IRenderPipeline : RenderPipelineBase
    {
        IRenderPipeline(EntityDatabase* entityDb, 
            AssetDatabase* assetDatabase,
            Sequencer* sequencer,
            IBatcher* batcher) : 
            RenderPipelineBase(entityDb, assetDatabase, sequencer, batcher) 
        {
        }

        protected:
            IRenderViewResources* GetViewResources(uint32_t index) final { return &m_resources[index]; }
            TResources m_resources[MAX_RENDER_VIEWS]{};
    };
}
