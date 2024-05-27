#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/LoggerPrintf.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ControlFlow/RemoteProcessRunner.h"
#include "Core/Input/InputSystem.h"
#include "Core/StringHashRegister.h"
#include "Core/Time.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "App/Engines/EngineCommandInput.h"
#include "App/Engines/EngineFlyCamera.h"
#include "App/Engines/EngineGatherRayTracingGeometry.h"
#include "App/Engines/EngineUpdateTransforms.h"
#include "App/Engines/EngineEntityCull.h"
#include "App/Engines/EngineDrawGeometry.h"
#include "App/Engines/EngineDebug.h"
#include "App/Engines/EngineScreenshot.h"
#include "App/Engines/EngineGizmos.h"
#include "App/Renderer/BatcherMeshStatic.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderPipelineDisptacher.h"
#include "App/Renderer/RenderPipelineScene.h"
#include "App/Renderer/RenderView.h"
#include "App/ApplicationConfig.h"
#include "Application.h"

namespace PK::App
{
    Application* Application::s_instance = nullptr;

    Application::Application(CArguments arguments, const std::string& name)
    {
        PK_LOG_SCOPE_TIMER(ApplicationCtor);
        PK_THROW_ASSERT(!s_instance, "Application already exists!");
        s_instance = this;

        m_logger = CreateRef<LoggerPrintf>();
        StaticLog::SetLogger(m_logger);
        
        PK_LOG_HEADER("----------INITIALIZING APPLICATION----------");
        
        {
            PK_LOG_VERBOSE("Running with '%u' arguments:", arguments.count);
            PK_LOG_SCOPE_INDENT(cargs);

            for (auto i = 0; i < arguments.count; ++i)
            {
                PK_LOG_VERBOSE("%s", arguments.args[i]);
            }
        }

        m_services = CreateScope<ServiceRegister>();
        m_services->Create<StringHashRegister>();
        m_services->Create<HashCache>();

        auto remoteProcessRunner = m_services->Create<RemoteProcessRunner>();
        auto cvariableRegister = m_services->Create<CVariableRegister>();
        auto entityDb = m_services->Create<EntityDatabase>();
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
        m_graphicsDriver = RHI::CreateDriver(workingDirectory.c_str(), RHIAPI::Vulkan);

        m_window = RHI::CreateWindowScope(WindowDescriptor(name + m_graphicsDriver->GetDriverHeader(),
            config->FileWindowIcon,
            config->InitialWidth,
            config->InitialHeight,
            config->EnableVsync,
            config->EnableCursor));

        m_window->SetOnCloseCallback([this]() { Close(); });

        assetDatabase->LoadDirectory<ShaderAsset>("res/shaders/");

        auto time = m_services->Create<Time>(sequencer, config->TimeScale, config->EnableFrameRateLog);
        auto inputSystem = m_services->Create<InputSystem>(sequencer);
        auto batcherMeshStatic = m_services->Create<BatcherMeshStatic>();
        auto renderPipelineDispatcher = m_services->Create<RenderPipelineDisptacher>(entityDb, assetDatabase, sequencer, batcherMeshStatic);
        auto renderPipelineScene = m_services->Create<RenderPipelineScene>(assetDatabase, config);

        renderPipelineDispatcher->SetRenderPipeline(RenderViewType::Scene, renderPipelineScene);

        auto engineFlyCamera = m_services->Create<EngineFlyCamera>(entityDb, keyConfig);
        auto engineUpdateTransforms = m_services->Create<EngineUpdateTransforms>(entityDb);
        auto engineCommands = m_services->Create<EngineCommandInput>(sequencer, keyConfig);
        auto engineEntityCull = m_services->Create<EngineEntityCull>(entityDb);
        auto engineDrawGeometry = m_services->Create<EngineDrawGeometry>(entityDb, sequencer);
        auto engineGatherRayTracingGeometry = m_services->Create<EngineGatherRayTracingGeometry>(entityDb);
        auto engineDebug = m_services->Create<EngineDebug>(assetDatabase, entityDb, batcherMeshStatic->GetMeshStaticCollection(), config);
        auto engineScreenshot = m_services->Create<EngineScreenshot>();
        auto engineGizmos = m_services->Create<EngineGizmos>(assetDatabase, sequencer, config);

        sequencer->SetSteps(
            {
                {
                    sequencer->GetRoot(),
                    {
                        Sequencer::Step::Create<ApplicationStep::OpenFrame>(time),
                        Sequencer::Step::Create<ApplicationStep::UpdateInput, RHIWindow*>(inputSystem),
                        Sequencer::Step::Create<ApplicationStep::UpdateEngines>(engineUpdateTransforms),
                        Sequencer::Step::Create<ApplicationStep::Render, RHIWindow*>(renderPipelineDispatcher),
                        Sequencer::Step::Create<ApplicationStep::Render, RHIWindow*>(engineScreenshot),
                        Sequencer::Step::Create<ApplicationStep::CloseFrame>(time),
                        Sequencer::Step::Create<ApplicationStep::CloseFrame, RHIWindow*>(inputSystem),
                        Sequencer::Step::Create<CArgumentsConst>(cvariableRegister),
                        Sequencer::Step::Create<CArgumentConst>(cvariableRegister),
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
                        Sequencer::Step::Create<RequestEntityCullRayTracingGeometry*>(engineGatherRayTracingGeometry),
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

        CVariableRegister::Create<CVariableFuncSimple>("Application.Close", [](){Application::Get().Close();});
        CVariableRegister::Create<CVariableFunc>("Application.VSync", [](const char** args, [[maybe_unused]] uint32_t count){Application::GetPrimaryWindow()->SetVSync((bool)atoi(args[0]));}, "0 = 0ff, 1 = On", 1u, 1u);
        CVariableRegister::Create<CVariableFuncSimple>("Application.VSync.Toggle", [](){Application::GetPrimaryWindow()->SetVSync(!Application::GetPrimaryWindow()->IsVSync());});

        // @TODO move this somewhere
        CVariableRegister::Create<CVariableFunc>("Application.Run.Executable", [remoteProcessRunner](const char** args, uint32_t count)
            {
                auto combinedArguments = std::string();

                for (auto i = 1u; i < count; ++i)
                {
                    combinedArguments.append(args[i]);
                    combinedArguments.append(" ");
                }

                remoteProcessRunner->ExecuteRemoteProcess({ args[0], combinedArguments.c_str() });
            }, "executable, arguments", 1u, 256u);

        PK_LOG_HEADER("----------APPLICATION INITIALIZED----------");
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

            RHI::GC();
        }
    }

    void Application::Close()
    {
        PK_LOG_INFO("Application.Close Requested.");
        m_isRunning = false;
    }
}