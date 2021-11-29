#include "PrecompiledHeader.h"
#include "Application.h"
#include "Utilities/Log.h"
#include "Utilities/StringHashID.h"
#include "Core/Input.h"
#include "Core/Time.h"
#include "Core/UpdateStep.h"
#include "Core/AssetDatabase.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Sequencer.h"
#include "Rendering/RenderPipeline.h"

namespace PK::Core
{
	using namespace Utilities;
	using namespace Rendering;

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name)
	{
		PK_THROW_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		uint logfilter = 0;
		logfilter |= PK::Utilities::Debug::PK_LOG_INFO;
		logfilter |= PK::Utilities::Debug::PK_LOG_ERROR;
		logfilter |= PK::Utilities::Debug::PK_LOG_WARNING;
		logfilter |= PK::Utilities::Debug::PK_LOG_VERBOSE;

		m_services = CreateScope<ServiceRegister>();
		m_services->Create<Utilities::Debug::Logger>(logfilter);
		m_services->Create<StringHashID>();
		auto entityDb = m_services->Create<PK::ECS::EntityDatabase>();
		auto sequencer = m_services->Create<PK::ECS::Sequencer>();
		auto assetDatabase = m_services->Create<AssetDatabase>(sequencer);
		auto time = m_services->Create<Time>(sequencer, 1.0f);
		auto input = m_services->Create<Input>(sequencer);
		
		m_graphicsDriver = GraphicsDriver::Create(APIType::Vulkan);

		m_window = Window::Create(WindowProperties(name, 1024, 512, true, true));
		Window::SetConsole(true);

		m_window->OnKeyInput = PK_BIND_MEMBER_FUNCTION(input, OnKeyInput);
		m_window->OnScrollInput = PK_BIND_MEMBER_FUNCTION(input, OnScrollInput);
		m_window->OnMouseButtonInput = PK_BIND_MEMBER_FUNCTION(input, OnMouseButtonInput);
		m_window->OnClose = PK_BIND_FUNCTION(Application::Close);

		auto renderPipeline = m_services->Create<RenderPipeline>(assetDatabase, 1024, 512);

		sequencer->SetSteps(
		{
			{
				sequencer->GetRoot(),
				{
					{ (int)UpdateStep::OpenFrame,		{ time }},
					{ (int)UpdateStep::UpdateInput,		{ PK_STEP_C(input, PK::Core::Window) } },
					{ (int)UpdateStep::Render,			{ PK_STEP_C(renderPipeline, PK::Core::Window) }},
					{ (int)UpdateStep::CloseFrame,		{ PK_STEP_C(input, PK::Core::Window), time }},
				}
			},
		});

		PK_LOG_HEADER("----------INITIALIZATION COMPLETE----------");
	}

	Application::~Application()
	{
		GetService<ECS::Sequencer>()->Release();
		GetService<AssetDatabase>()->Unload();
		m_services->Clear();
		m_window = nullptr;
		m_graphicsDriver = nullptr;
	}

	void Application::Run()
	{
		auto sequencer = GetService<ECS::Sequencer>();

		while (m_window->IsAlive() && m_Running)
		{
			m_window->PollEvents();

			if (m_window->IsMinimized())
			{
				continue;
			}

			m_window->Begin();

			sequencer->InvokeRootStep<PK::Core::Window>(m_window.get(), (int)UpdateStep::OpenFrame);
			sequencer->InvokeRootStep<PK::Core::Window>(m_window.get(), (int)UpdateStep::UpdateInput);
			sequencer->InvokeRootStep<PK::Core::Window>(m_window.get(), (int)UpdateStep::UpdateEngines);
			sequencer->InvokeRootStep<PK::Core::Window>(m_window.get(), (int)UpdateStep::Render);
			sequencer->InvokeRootStep<PK::Core::Window>(m_window.get(), (int)UpdateStep::CloseFrame);
			
			m_window->End();
		}
	}

	void Application::Close()
	{
		m_Running = false;
	}
}