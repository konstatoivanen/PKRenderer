#include "PrecompiledHeader.h"
#include "Application.h"
#include "Utilities/Log.h"
#include "Utilities/StringHashID.h"
#include "Core/Input.h"
#include "Core/Time.h"
#include "Core/ApplicationConfig.h"
#include "Core/CommandConfig.h"
#include "Core/UpdateStep.h"
#include "Core/AssetDatabase.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Sequencer.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/HashCache.h"
#include "ECS/Contextual/Engines/EngineCommandInput.h"
#include "ECS/Contextual/Engines/EngineEditorCamera.h"
#include "ECS/Contextual/Tokens/TimeToken.h"

namespace PK::Core
{
    using namespace Utilities;
    using namespace Rendering;

    Application* Application::s_Instance = nullptr;

    Application::Application(ApplicationArguments arguments, const std::string& name)
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
        m_services->Create<HashCache>();
        auto entityDb = m_services->Create<PK::ECS::EntityDatabase>();
        auto sequencer = m_services->Create<PK::ECS::Sequencer>();
        auto assetDatabase = m_services->Create<AssetDatabase>(sequencer);

        assetDatabase->LoadDirectory<ApplicationConfig>("res/configs/");
        assetDatabase->LoadDirectory<CommandConfig>("res/configs/");
        auto config = assetDatabase->Find<ApplicationConfig>("Active");
        auto commandConfig = assetDatabase->Find<CommandConfig>("Active");

        auto time = m_services->Create<Time>(sequencer, config->TimeScale);
        auto input = m_services->Create<Input>(sequencer);
        
        m_graphicsDriver = GraphicsDriver::Create(APIType::Vulkan);

        m_window = Window::Create(WindowProperties(name, config->InitialWidth, config->InitialHeight, config->EnableVsync, config->EnableCursor));
        Window::SetConsole(config->EnableConsole);
        m_window->OnKeyInput = PK_BIND_MEMBER_FUNCTION(input, OnKeyInput);
        m_window->OnScrollInput = PK_BIND_MEMBER_FUNCTION(input, OnScrollInput);
        m_window->OnMouseButtonInput = PK_BIND_MEMBER_FUNCTION(input, OnMouseButtonInput);
        m_window->OnClose = PK_BIND_FUNCTION(Application::Close);

        assetDatabase->LoadDirectory<Shader>("res/shaders/");

        auto engineEditorCamera = m_services->Create<ECS::Engines::EngineEditorCamera>(sequencer, time, config);
        auto renderPipeline = m_services->Create<RenderPipeline>(assetDatabase, config);
        auto engineCommands = m_services->Create<ECS::Engines::EngineCommandInput>(assetDatabase, sequencer, time, entityDb, commandConfig);

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
            {
                time,
                {
                    PK_STEP_T(renderPipeline, PK::ECS::Tokens::TimeToken)
                }
            },
            {
                input,
                {
                    PK_STEP_T(engineCommands, Input),
                    PK_STEP_T(engineEditorCamera, Input),
                }
            },
            {
                engineCommands,
                {
                    PK_STEP_T(engineEditorCamera, ConsoleCommandToken),
                    //PK_STEP_T(gizmoRenderer, ConsoleCommandToken),
                    //PK_STEP_T(engineScreenshot, ConsoleCommandToken),
                }
            },
            {
                engineEditorCamera,
                {
                    PK_STEP_T(renderPipeline, PK::ECS::Tokens::ViewProjectionUpdateToken)
                }
            }
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

            sequencer->Next((int)UpdateStep::OpenFrame);

            m_window->Begin();

            sequencer->Next<PK::Core::Window>(m_window.get(), (int)UpdateStep::OpenFrame);
            sequencer->Next<PK::Core::Window>(m_window.get(), (int)UpdateStep::UpdateInput);
            sequencer->Next<PK::Core::Window>(m_window.get(), (int)UpdateStep::UpdateEngines);
            sequencer->Next<PK::Core::Window>(m_window.get(), (int)UpdateStep::Render);
            sequencer->Next<PK::Core::Window>(m_window.get(), (int)UpdateStep::CloseFrame);
            
            m_window->End();
            
            sequencer->Next((int)UpdateStep::CloseFrame);

            GraphicsAPI::GC();
        }
    }

    void Application::Close()
    {
        m_Running = false;
    }
}