#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/TimeFrameInfo.h"
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Rendering/Geometry/IBatcher.h"
#include "Rendering/IRenderPipeline.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI, struct Window)

namespace PK::Rendering
{
	class RenderPipelineDisptacher : public Core::Services::IService,
		public Core::ControlFlow::IStepApplicationRenderWindow,
		public Core::ControlFlow::IStep<PK::Core::Services::TimeFrameInfo*>
	{
		public:
			constexpr static size_t MAX_SCENE_VIEWS = 32ull;

			RenderPipelineDisptacher(ECS::EntityDatabase* entityDb,
				Core::Assets::AssetDatabase* assetDatabase,
				Core::ControlFlow::Sequencer* sequencer, 
				Geometry::IBatcher* batcher);

			inline void SetRenderPipeline(Rendering::RenderViewType type, IRenderPipeline* pipeline) { m_renderPipelines[(int)type] = pipeline; }

			// Core::ControlFlow::IStep<PK::Core::Services::TimeFrameInfo*>
			virtual void Step(PK::Core::Services::TimeFrameInfo* token) final { m_timeFrameInfo = *token;}

			// Core::ControlFlow::IStepApplicationRenderWindow
			virtual void OnApplicationRender(Rendering::RHI::Window* window) final;

		private:
			Core::ControlFlow::Sequencer* m_sequencer = nullptr;
			ECS::EntityDatabase* m_entityDb = nullptr;
			Geometry::IBatcher* m_batcher;
			IRenderPipeline* m_renderPipelines[(int)Rendering::RenderViewType::Count]{};
			Rendering::Objects::RenderView m_renderViews[MAX_SCENE_VIEWS]{};
			uint32_t m_renderViewCount;
			
			PK::Core::Services::TimeFrameInfo m_timeFrameInfo{};
	};
}