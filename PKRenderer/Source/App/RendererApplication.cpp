#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/LoggerPrintf.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ControlFlow/RemoteProcessRunner.h"
#include "Core/Input/InputSystem.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Core/IApplication.h"
#include "App/Engines/EngineTime.h"
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
#include "App/RendererConfig.h"
#include "RendererApplication.h"

namespace PK::App
{
    RendererApplication::RendererApplication(const CArguments& arguments) :
        IApplication(arguments, "PK Renderer", CreateRef<LoggerPrintf>())
    {
        PK_LOG_SCOPE_TIMER(ApplicationCtor);
        PK_LOG_HEADER("----------RendererApplication.Ctor Begin----------");
        PK_LOG_SCOPE_INDENT();

        CVariableRegister::Create<CVariableFunc>("Application.VSync", [](const char** args, [[maybe_unused]] uint32_t count)
            {
                IApplication::Get()->GetPrimaryWindow()->SetVSync((bool)atoi(args[0]));
            }, "0 = 0ff, 1 = On", 1u, 1u);

        CVariableRegister::Create<CVariableFuncSimple>("Application.VSync.Toggle", []()
            {
                IApplication::Get()->GetPrimaryWindow()->SetVSync(!IApplication::Get()->GetPrimaryWindow()->IsVSync());
            });

        CVariableRegister::Create<CVariableFunc>("Application.Fullscreen", [](const char** args, [[maybe_unused]] uint32_t count)
            {
                IApplication::Get()->GetPrimaryWindow()->SetFullscreen((bool)atoi(args[0]));
            }, "0 = 0ff, 1 = On", 1u, 1u);

        CVariableRegister::Create<CVariableFuncSimple>("Application.Fullscreen.Toggle", []()
            {
                IApplication::Get()->GetPrimaryWindow()->SetFullscreen(!IApplication::Get()->GetPrimaryWindow()->IsFullscreen());
            });

        GetServices()->Create<HashCache>();

        auto remoteProcessRunner = GetServices()->Create<RemoteProcessRunner>();
        auto entityDb = GetServices()->Create<EntityDatabase>();
        auto sequencer = GetServices()->Create<Sequencer>();
        auto assetDatabase = GetServices()->Create<AssetDatabase>(sequencer);

        assetDatabase->LoadDirectory<RendererConfig>("Content/Configs/");
        auto config = assetDatabase->Find<RendererConfig>("Active");

        assetDatabase->LoadDirectory<InputKeyConfig>("Content/Configs/");
        auto keyConfig = assetDatabase->Find<InputKeyConfig>("Active");

        uint32_t logfilter = 0u;
        logfilter |= config->EnableLogRHI ? PK_LOG_LVL_RHI : 0u;
        logfilter |= config->EnableLogVerbose ? PK_LOG_LVL_VERBOSE : 0u;
        logfilter |= config->EnableLogInfo ? PK_LOG_LVL_INFO : 0u;
        logfilter |= config->EnableLogWarning ? PK_LOG_LVL_WARNING : 0u;
        logfilter |= config->EnableLogError ? PK_LOG_LVL_ERROR : 0u;
        StaticLog::SetSeverityMask((LogSeverity)logfilter);
        StaticLog::SetShowConsole(config->EnableConsole);

        m_graphicsDriver = RHI::CreateDriver(GetWorkingDirectory().c_str(), RHIAPI::Vulkan);

        m_window = RHI::CreateWindowScope(WindowDescriptor(GetName() + m_graphicsDriver->GetDriverHeader(),
            config->FileWindowIcon,
            config->InitialWidth,
            config->InitialHeight,
            config->EnableVsync,
            config->EnableCursor));

        m_window->SetOnCloseCallback([this]() { Close(); });

        assetDatabase->LoadDirectory<ShaderAsset>("Content/Shaders/");

        auto time = GetServices()->Create<EngineTime>(sequencer, config->TimeScale, config->EnableFrameRateLog);
        auto inputSystem = GetServices()->Create<InputSystem>(sequencer);
        auto batcherMeshStatic = GetServices()->Create<BatcherMeshStatic>();
        auto renderPipelineDispatcher = GetServices()->Create<RenderPipelineDisptacher>(entityDb, assetDatabase, sequencer, batcherMeshStatic);
        auto renderPipelineScene = GetServices()->Create<RenderPipelineScene>(assetDatabase, config);

        renderPipelineDispatcher->SetRenderPipeline(RenderViewType::Scene, renderPipelineScene);

        auto engineFlyCamera = GetServices()->Create<EngineFlyCamera>(entityDb, keyConfig);
        auto engineUpdateTransforms = GetServices()->Create<EngineUpdateTransforms>(entityDb);
        auto engineCommands = GetServices()->Create<EngineCommandInput>(sequencer, keyConfig);
        auto engineEntityCull = GetServices()->Create<EngineEntityCull>(entityDb);
        auto engineDrawGeometry = GetServices()->Create<EngineDrawGeometry>(entityDb, sequencer);
        auto engineGatherRayTracingGeometry = GetServices()->Create<EngineGatherRayTracingGeometry>(entityDb);
        auto engineDebug = GetServices()->Create<EngineDebug>(assetDatabase, entityDb, batcherMeshStatic->GetMeshStaticCollection(), config);
        auto engineScreenshot = GetServices()->Create<EngineScreenshot>();
        auto engineGizmos = GetServices()->Create<EngineGizmos>(assetDatabase, sequencer, config);

        auto cvariableRegister = GetService<CVariableRegister>();

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
                        Sequencer::Step::Create<AssetImportEvent<RendererConfig>*>(renderPipelineScene),
                        Sequencer::Step::Create<AssetImportEvent<RendererConfig>*>(engineGizmos),
                        Sequencer::Step::Create<AssetImportEvent<RendererConfig>*>(engineDebug),
                        Sequencer::Step::Create<AssetImportEvent<InputKeyConfig>*>(engineFlyCamera),
                        Sequencer::Step::Create<AssetImportEvent<InputKeyConfig>*>(engineCommands)
                    }
                },
            });

        PK_LOG_HEADER("----------RendererApplication.Ctor End----------");
    }

    RendererApplication::~RendererApplication()
    {
        GetService<Sequencer>()->Release();
        GetService<AssetDatabase>()->Unload();
        GetServices()->Clear();
        m_window = nullptr;
        m_graphicsDriver = nullptr;
        PK_LOG_HEADER("----------RendererApplication.Dtor----------");
    }

    void RendererApplication::Execute()
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

    void RendererApplication::Close()
    {
        PK_LOG_INFO("Application.Close Requested.");
        m_isRunning = false;
    }
}

PK::IApplication* PK::CreateProjectApplication(const PK::CArguments& arguments)
{
    return new App::RendererApplication(arguments);
}