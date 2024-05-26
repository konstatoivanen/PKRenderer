#pragma once
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Core/TimeFrameInfo.h"
#include "App/Renderer/IBatcher.h"
#include "App/Renderer/IRenderPipeline.h"
#include "App/Renderer/RenderView.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class Sequencer)

namespace PK::App
{
	class RenderPipelineDisptacher :
		public IStepApplicationRenderWindow,
		public IStep<TimeFrameInfo*>
	{
		public:
			constexpr static size_t MAX_SCENE_VIEWS = 32ull;

			RenderPipelineDisptacher(EntityDatabase* entityDb,
				AssetDatabase* assetDatabase,
				Sequencer* sequencer, 
				IBatcher* batcher);

			inline void SetRenderPipeline(RenderViewType type, IRenderPipeline* pipeline) { m_renderPipelines[(int)type] = pipeline; }

			virtual void Step(TimeFrameInfo* token) final { m_timeFrameInfo = *token;}

			virtual void OnApplicationRender(RHIWindow* window) final;

		private:
			Sequencer* m_sequencer = nullptr;
			EntityDatabase* m_entityDb = nullptr;
			IBatcher* m_batcher;
			IRenderPipeline* m_renderPipelines[(int)RenderViewType::Count]{};
			RenderView m_renderViews[MAX_SCENE_VIEWS]{};
			uint32_t m_renderViewCount;
			
			TimeFrameInfo m_timeFrameInfo{};
	};
}