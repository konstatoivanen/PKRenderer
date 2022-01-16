#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/Window.h"
#include "Utilities/MemoryBlock.h"
#include "Rendering/Objects/Buffer.h"

namespace PK::ECS::Engines
{
	using namespace PK::Utilities;
	using namespace PK::Rendering::Objects;

	class EngineScreenshot : public IService, public IStep<Window>
	{
		public:
			EngineScreenshot();
			void Step(Window* window) override final;
		
		private:
			MemoryBlock<char> m_frameData;
			Ref<Buffer> m_copyBuffer;
			uint32_t m_frameCounter = 0u;
	};
}