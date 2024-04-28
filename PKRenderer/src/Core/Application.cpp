#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/LoggerPrintf.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Core/ControlFlow/RemoteProcessRunner.h"
#include "Core/Input/InputSystem.h"
#include "Core/Services/StringHashID.h"
#include "Core/Services/Time.h"
#include "Core/ApplicationConfig.h"
#include "ECS/EntityDatabase.h"
#include "Engines/EngineCommandInput.h"
#include "Engines/EngineFlyCamera.h"
#include "Engines/EngineGatherRayTracingGeometry.h"
#include "Engines/EngineUpdateTransforms.h"
#include "Engines/EngineEntityCull.h"
#include "Engines/EngineDrawGeometry.h"
#include "Engines/EngineDebug.h"
#include "Engines/EngineScreenshot.h"
#include "Engines/EngineGizmos.h"
#include "Rendering/Objects/RenderView.h"
#include "Rendering/Geometry/BatcherStaticMesh.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/HashCache.h"
#include "Rendering/RenderPipelineDisptacher.h"
#include "Rendering/RenderPipelineScene.h"
#include "Application.h"

namespace PK::Core
{
    using namespace PK::Utilities;
    using namespace PK::Core::Assets;
    using namespace PK::Core::CLI;
    using namespace PK::Core::ControlFlow;
    using namespace PK::Core::Input;
    using namespace PK::Core::Services;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Geometry;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    Application* Application::s_instance = nullptr;

    Application::Application(CLI::CArguments arguments, const std::string& name)
    {
        PK_THROW_ASSERT(!s_instance, "Application already exists!");
        s_instance = this;

        m_logger = CreateRef<LoggerPrintf>();
        StaticLog::SetLogger(m_logger);

        m_services = CreateScope<ServiceRegister>();
        m_services->Create<StringHashID>();
        m_services->Create<HashCache>();

        auto remoteProcessRunner = m_services->Create<RemoteProcessRunner>();
        auto cvariableRegister = m_services->Create<CVariableRegister>();
        auto entityDb = m_services->Create<PK::ECS::EntityDatabase>();
        auto sequencer = m_services->Create<Sequencer>();
        auto assetDatabase = m_services->Create<AssetDatabase>(sequencer);

        assetDatabase->LoadDirectory<ApplicationConfig>("res/configs/");
        auto config = assetDatabase->Find<ApplicationConfig>("Active");

        assetDatabase->LoadDirectory<InputKeyConfig>("res/configs/");
        auto keyConfig = assetDatabase->Find<InputKeyConfig>("Active");

        uint32_t logfilter = 0u;
        logfilter |= config->EnableLogRHI ? PK_LOG_LVL_RHI : 0u;
        logfilter |= config->EnableLogVerbose ? PK_LOG_LVL_VERBOSE : 0u;
        logfilter |= config->EnableLogInfo ? PK_LOG_LVL_INFO : 0u;
        logfilter |= config->EnableLogWarning ? PK_LOG_LVL_WARNING : 0u;
        logfilter |= config->EnableLogError ? PK_LOG_LVL_ERROR : 0u;
        m_logger->SetSeverityMask((LogSeverity)logfilter);
        m_logger->SetShowConsole(config->EnableConsole);

        auto workingDirectory = std::filesystem::path(arguments.args[0]).remove_filename().string();
        m_graphicsDriver = RHI::CreateRHIDriver(workingDirectory, APIType::Vulkan);

        m_window = Window::Create(WindowProperties(name + m_graphicsDriver->GetDriverHeader(),
            config->FileWindowIcon,
            config->InitialWidth,
            config->InitialHeight,
            config->EnableVsync,
            config->EnableCursor));

        m_window->OnClose = PK_BIND_FUNCTION(this, Application::Close);

        assetDatabase->LoadDirectory<Shader>("res/shaders/");

        auto time = m_services->Create<Time>(sequencer, config->TimeScale, config->EnableFrameRateLog);
        auto inputSystem = m_services->Create<InputSystem>(sequencer);
        auto batcherStaticMesh = m_services->Create<BatcherStaticMesh>(assetDatabase);
        auto renderPipelineDispatcher = m_services->Create<RenderPipelineDisptacher>(entityDb, assetDatabase, sequencer, batcherStaticMesh);
        auto renderPipelineScene = m_services->Create<RenderPipelineScene>(entityDb, assetDatabase, config);
        
        renderPipelineDispatcher->SetRenderPipeline(RenderViewType::Scene, renderPipelineScene);

        auto engineFlyCamera = m_services->Create<Engines::EngineFlyCamera>(entityDb, keyConfig);
        auto engineUpdateTransforms = m_services->Create<Engines::EngineUpdateTransforms>(entityDb);
        auto engineCommands = m_services->Create<Engines::EngineCommandInput>(sequencer, keyConfig, arguments);
        auto engineEntityCull = m_services->Create<Engines::EngineEntityCull>(entityDb);
        auto engineDrawGeometry = m_services->Create<Engines::EngineDrawGeometry>(entityDb, sequencer);
        auto engineGatherRayTracingGeometry = m_services->Create<Engines::EngineGatherRayTracingGeometry>(entityDb);
        auto engineDebug = m_services->Create<Engines::EngineDebug>(assetDatabase, entityDb, batcherStaticMesh->GetStaticSceneMesh(), config);
        auto engineScreenshot = m_services->Create<Engines::EngineScreenshot>();
        auto engineGizmos = m_services->Create<Engines::EngineGizmos>(assetDatabase, sequencer, config);

        sequencer->SetSteps(
            {
                {
                    sequencer->GetRoot(),
                    {
                        Sequencer::Step::Create<ApplicationStep::OpenFrame>(time),
                        Sequencer::Step::Create<ApplicationStep::UpdateInput, Window*>(inputSystem),
                        Sequencer::Step::Create<ApplicationStep::UpdateEngines>(engineUpdateTransforms),
                        Sequencer::Step::Create<ApplicationStep::Render, Window*>(renderPipelineDispatcher),
                        Sequencer::Step::Create<ApplicationStep::Render, Window*>(engineScreenshot),
                        Sequencer::Step::Create<ApplicationStep::CloseFrame>(time),
                        Sequencer::Step::Create<ApplicationStep::CloseFrame, Window*>(inputSystem),
                        Sequencer::Step::Create<CLI::CArgumentsConst>(cvariableRegister),
                        Sequencer::Step::Create<CLI::CArgumentConst>(cvariableRegister),
                        Sequencer::Step::Create<RemoteProcessCommand*>(remoteProcessRunner)
                    }
                },
                {
                    time,
                    {
                        Sequencer::Step::Create<TimeFrameInfo*>(renderPipelineDispatcher),
                        Sequencer::Step::Create<TimeFrameInfo*>(engineFlyCamera),
                    }
                },
                {
                    inputSystem,
                    {
                        Sequencer::Step::Create<InputDevice*>(engineCommands),
                        Sequencer::Step::Create<InputDevice*>(engineFlyCamera),
                    }
                },
                {
                    renderPipelineScene,
                    {
                        Sequencer::Step::Create<RequestRayTracingGeometry*>(engineGatherRayTracingGeometry),
                        Sequencer::Step::Create<RequestEntityCullFrustum*>(engineEntityCull),
                        Sequencer::Step::Create<RequestEntityCullCascades*>(engineEntityCull),
                        Sequencer::Step::Create<RequestEntityCullCubeFaces*>(engineEntityCull),
                        Sequencer::Step::Create<RenderPipelineEvent*>(engineGizmos),
                        Sequencer::Step::Create<RenderPipelineEvent*>(engineDrawGeometry),
                    }
                },
                {
                    engineDrawGeometry,
                    {
                        Sequencer::Step::Create<RequestEntityCullFrustum*>(engineEntityCull)
                    }
                },
                {
                    engineGizmos,
                    {
                        Sequencer::Step::Create<IGizmos*>(engineDebug)
                    }
                },
                {
                    assetDatabase,
                    {
                        Sequencer::Step::Create<AssetImportEvent<ApplicationConfig>*>(renderPipelineScene),
                        Sequencer::Step::Create<AssetImportEvent<ApplicationConfig>*>(engineGizmos),
                        Sequencer::Step::Create<AssetImportEvent<ApplicationConfig>*>(engineDebug),
                        Sequencer::Step::Create<AssetImportEvent<InputKeyConfig>*>(engineFlyCamera),
                        Sequencer::Step::Create<AssetImportEvent<InputKeyConfig>*>(engineCommands)
                    }
                },
            });

        CVariableRegister::Create<CVariableFunc>("Application.Close", [](const char** args, uint32_t count) { Application::Get().Close(); });
        CVariableRegister::Create<CVariableFunc>("Application.VSync", [](const char** args, uint32_t count) { Application::GetPrimaryWindow()->SetVSync((bool)atoi(args[0])); }, "0 = 0ff, 1 = On", 1u, 1u);
        CVariableRegister::Create<CVariableFunc>("Application.VSync.Toggle", [](const char** args, uint32_t count) { Application::GetPrimaryWindow()->SetVSync(!Application::GetPrimaryWindow()->IsVSync()); });

        // PKAssetTools execute cvar binding
        if (arguments.count >= 4)
        {
            auto workingdir = std::filesystem::path(arguments.args[0]).string();
            auto executablePath = std::filesystem::path(arguments.args[1]).string();
            auto sourcedirectory = std::filesystem::path(arguments.args[2]).string();
            auto targetdirectory = std::filesystem::path(arguments.args[3]).string();
            auto executableArgs = std::string("\"" + sourcedirectory + "\" \"" + targetdirectory + "");

            CVariableRegister::Create<CVariableFunc>("Application.Run.PKAssetTools", [remoteProcessRunner, executablePath, executableArgs](const char** args, uint32_t count)
            {
                remoteProcessRunner->ExecuteRemoteProcess({ executablePath.c_str(), executableArgs.c_str() });
            });
        }

        PK_LOG_HEADER("----------INITIALIZATION COMPLETE----------");
    }

    Application::~Application()
    {
        GetService<Sequencer>()->Release();
        GetService<AssetDatabase>()->Unload();
        m_services->Clear();
        m_window = nullptr;
        m_graphicsDriver = nullptr;
        PK_LOG_HEADER("----------APPLICATION TERMINATED----------");
    }

    void Application::Execute()
    {
        auto sequencer = GetService<Sequencer>();

        while (m_window->IsAlive() && m_isRunning)
        {
            m_window->PollEvents();

            if (m_window->IsMinimized())
            {
                m_window->WaitEvents();
                continue;
            }

            sequencer->NextRoot(ApplicationStep::OpenFrame());

            m_window->Begin();
            sequencer->NextRoot(ApplicationStep::OpenFrame(), m_window.get());
            sequencer->NextRoot(ApplicationStep::UpdateInput(), m_window.get());
            sequencer->NextRoot(ApplicationStep::UpdateEngines());
            sequencer->NextRoot(ApplicationStep::Render(), m_window.get());
            sequencer->NextRoot(ApplicationStep::CloseFrame(), m_window.get());
            m_window->End();

            sequencer->NextRoot(ApplicationStep::CloseFrame());

            GraphicsAPI::GC();
        }
    }

    void Application::Close()
    {
        PK_LOG_INFO("Application.Close Requested.");
        m_isRunning = false;
    }
}