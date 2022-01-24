#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/Window.h"
#include "Core/ConsoleCommandBinding.h"
#include "Utilities/MemoryBlock.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Structs/ExecutionGate.h"

namespace PK::ECS::Engines
{
	using namespace PK::Utilities;
	using namespace PK::Rendering::Objects;

	class EngineScreenshot : public IService, public IStep<Window>, public IStep<ConsoleCommandToken>
	{
		public:
			EngineScreenshot();
			void Step(Window* window) override final;
			void Step(ConsoleCommandToken* token) override final;
		
		private:
			ExecutionGate m_copyGate;
			MemoryBlock<ushort> m_accumulatedPixels;
			Ref<Buffer> m_copyBuffer;
			uint32_t m_captureCounter = 0u;
			uint32_t m_captureFrameCount = 0u;
			uint2 m_captureResolution = PK_UINT2_ZERO;
			uint2 m_currentResolution = PK_UINT2_ZERO;
	};
}