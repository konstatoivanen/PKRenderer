#pragma once
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Core/IService.h"
#include "Core/TimeFrameInfo.h"
#include "Renderer/IBatcher.h"
#include "Renderer/IRenderPipeline.h"
#include "Renderer/RenderView.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)

namespace PK::Renderer
{
	class RenderPipelineDisptacher : public Core::IService,
		public Core::ControlFlow::IStepApplicationRenderWindow,
		public Core::ControlFlow::IStep<PK::Core::TimeFrameInfo*>
	{
		public:
			constexpr static size_t MAX_SCENE_VIEWS = 32ull;

			RenderPipelineDisptacher(ECS::EntityDatabase* entityDb,
				Core::Assets::AssetDatabase* assetDatabase,
				Core::ControlFlow::Sequencer* sequencer, 
				IBatcher* batcher);

			inline void SetRenderPipeline(RenderViewType type, IRenderPipeline* pipeline) { m_renderPipelines[(int)type] = pipeline; }

			virtual void Step(Core::TimeFrameInfo* token) final { m_timeFrameInfo = *token;}

			virtual void OnApplicationRender(Graphics::Window* window) final;

		private:
			Core::ControlFlow::Sequencer* m_sequencer = nullptr;
			ECS::EntityDatabase* m_entityDb = nullptr;
			IBatcher* m_batcher;
			IRenderPipeline* m_renderPipelines[(int)RenderViewType::Count]{};
			RenderView m_renderViews[MAX_SCENE_VIEWS]{};
			uint32_t m_renderViewCount;
			
			Core::TimeFrameInfo m_timeFrameInfo{};
	};
}