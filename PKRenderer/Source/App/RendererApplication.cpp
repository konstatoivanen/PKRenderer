#include "PrecompiledHeader.h"
#include <chrono>
#include <thread>
#include "Core/ECS/EntityDatabase.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/LoggerPrintf.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ControlFlow/RemoteProcessRunner.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/MeshStaticAsset.h"
#include "Core/Rendering/Window.h"
#include "Core/IApplication.h"
#include "App/FrameStep.h"
#include "App/FrameContext.h"
#include "App/Engines/EngineInput.h"
#include "App/Engines/EngineTime.h"
#include "App/Engines/EngineCommandInput.h"
#include "App/Engines/EngineViewUpdate.h"
#include "App/Engines/EngineFlyCamera.h"
#include "App/Engines/EngineGatherRayTracingGeometry.h"
#include "App/Engines/EngineUpdateTransforms.h"
#include "App/Engines/EngineEntityCull.h"
#include "App/Engines/EngineDrawGeometry.h"
#include "App/Engines/EngineDebug.h"
#include "App/Engines/EngineScreenshot.h"
#include "App/Engines/EngineGUIRenderer.h"
#include "App/Engines/EngineProfiler.h"
#include "App/Renderer/BatcherMeshStatic.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderPipelineScene.h"
#include "App/Renderer/RenderView.h"
#include "App/BaseRendererConfig.h"
#include "RendererApplication.h"

namespace PK::App
{
    // @TODO Idea
    // Separate a windowed application into a different base class that will handle
    // - Windowing
    // - RHI
    // - Input
    // - Time
    RendererApplication::RendererApplication(const CArguments& arguments) :
        IApplication(arguments, "PK Renderer", CreateRef<LoggerPrintf>())
    {
        PK_LOG_TIMER_FUNC();
        PK_LOG_HEADER_SCOPE("----------RendererApplication.Ctor Begin----------");

        BaseRendererConfig config("Content/Configs/BaseRenderer.cfg");
        m_inactiveFrameInterval = config.InactiveFrameInterval;
        m_RHIDriver = RHI::CreateDriver(GetWorkingDirectory(), config.RHIDesc);

        config.WindowDesc.title = { GetName(), m_RHIDriver->GetDriverHeader() };
        m_window = CreateUnique<Window>(config.WindowDesc);
        m_window->SetOnCloseCallback([this]() { Close(); });

        GetServices()->Create<HashCache>();

        auto sequencer = GetServices()->Create<Sequencer>();
        auto entityDb = GetServices()->Create<EntityDatabase>();
        auto assetDatabase = GetServices()->Create<AssetDatabase>(sequencer);
        auto input = GetServices()->Create<EngineInput>(sequencer);
        auto time = GetServices()->Create<EngineTime>(sequencer, config.TimeScale);
        Platform::SetInputHandler(input);

        assetDatabase->LoadDirectory<ShaderAsset>("Content/Shaders/");

        auto batcherMeshStatic = GetServices()->Create<BatcherMeshStatic>();
        assetDatabase->RegisterFactory<MeshStaticAsset>(batcherMeshStatic);

        auto renderPipelineScene = GetServices()->Create<RenderPipelineScene>(assetDatabase, entityDb, sequencer, batcherMeshStatic);

        auto inputConfig = assetDatabase->Load<InputKeyConfig>("Content/Configs/Input.keycfg").get();
        auto remoteProcessRunner = GetServices()->Create<RemoteProcessRunner>();
        auto engineViewUpdate = GetServices()->Create<EngineViewUpdate>(sequencer, entityDb);
        auto engineCommands = GetServices()->Create<EngineCommandInput>(sequencer, inputConfig);
        auto engineUpdateTransforms = GetServices()->Create<EngineUpdateTransforms>(entityDb);
        auto engineEntityCull = GetServices()->Create<EngineEntityCull>(entityDb);
        auto engineDrawGeometry = GetServices()->Create<EngineDrawGeometry>(entityDb, sequencer);
        auto engineGatherRayTracingGeometry = GetServices()->Create<EngineGatherRayTracingGeometry>(entityDb);
        auto engineScreenshot = GetServices()->Create<EngineScreenshot>();
        auto engineGUIRenderer = GetServices()->Create<EngineGUIRenderer>(assetDatabase, sequencer);
        auto engineProfiler = GetServices()->Create<EngineProfiler>(assetDatabase);

        auto engineFlyCamera = GetServices()->Create<EngineFlyCamera>(entityDb, inputConfig);
        auto engineDebug = GetServices()->Create<EngineDebug>(assetDatabase, entityDb, batcherMeshStatic->GetMeshStaticCollection());
        
        auto cvariableRegister = GetService<CVariableRegister>();

        sequencer->SetSteps(
            {
                {
                    sequencer->GetRoot(),
                    {
                        Sequencer::Step::Create<FrameStep::Initialize, FrameContext*>(time),
                        Sequencer::Step::Create<FrameStep::Update, FrameContext*>(input),
                        Sequencer::Step::Create<FrameStep::Update, FrameContext*>(engineCommands),
                        Sequencer::Step::Create<FrameStep::Update, FrameContext*>(engineDebug),
                        Sequencer::Step::Create<FrameStep::Update, FrameContext*>(engineViewUpdate),
                        Sequencer::Step::Create<FrameStep::Update, FrameContext*>(engineFlyCamera),
                        Sequencer::Step::Create<FrameStep::Update, FrameContext*>(engineUpdateTransforms),
                        Sequencer::Step::Create<FrameStep::Render, FrameContext*>(renderPipelineScene),
                        Sequencer::Step::Create<FrameStep::Render, FrameContext*>(engineScreenshot),
                        Sequencer::Step::Create<FrameStep::Finalize, FrameContext*>(time),

                        Sequencer::Step::Create<CArgumentsConst>(cvariableRegister),
                        Sequencer::Step::Create<CArgumentConst>(cvariableRegister),
                        Sequencer::Step::Create<RemoteProcessCommand*>(remoteProcessRunner)
                    }
                },
                {
                    time,
                    {
                        Sequencer::Step::Create<TimeFramerateInfo*>(engineProfiler)
                    }
                },
                {
                    renderPipelineScene,
                    {
                        Sequencer::Step::Create<RequestEntityCullRayTracingGeometry*>(engineGatherRayTracingGeometry),
                        Sequencer::Step::Create<RequestEntityCullFrustum*>(engineEntityCull),
                        Sequencer::Step::Create<RequestEntityCullCascades*>(engineEntityCull),
                        Sequencer::Step::Create<RequestEntityCullCubeFaces*>(engineEntityCull),
                        Sequencer::Step::Create<RenderPipelineEvent*>(engineGUIRenderer),
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
                    engineGUIRenderer,
                    {
                        Sequencer::Step::Create<IGizmosRenderer*>(engineDebug),
                        Sequencer::Step::Create<IGUIRenderer*>(engineProfiler)
                    }
                },
                {
                    assetDatabase,
                    {
                        Sequencer::Step::Create<AssetImportEvent<EngineDebugConfig>*>(engineDebug),
                        Sequencer::Step::Create<AssetImportEvent<InputKeyConfig>*>(engineFlyCamera),
                        Sequencer::Step::Create<AssetImportEvent<InputKeyConfig>*>(engineCommands)
                    }
                },
            });

        CVariableRegister::Create<CVariableFunc>("Application.VSync", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                auto mode = RHIEnumConvert::StringToVSyncMode(args[0]);
                IApplication::Get()->GetPrimaryWindow()->SetVSync(mode);
            }, "See VSyncMode enum for value names", 1u);

        CVariableRegister::Create<CVariableFuncSimple>("Application.VSync.Cycle", []()
            {
                // Only cycle multi backing frame modes.
                auto current = (uint32_t)IApplication::Get()->GetPrimaryWindow()->GetVSyncMode();
                current = (current + 1u) % (uint32_t)VSyncMode::SharedDemandRefresh;
                IApplication::Get()->GetPrimaryWindow()->SetVSync((VSyncMode)current);
            });

        CVariableRegister::Create<CVariableFunc>("Application.Fullscreen", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                IApplication::Get()->GetPrimaryWindow()->SetFullscreen((bool)atoi(args[0]));
            }, "0 = 0ff, 1 = On", 1u);

        CVariableRegister::Create<CVariableFuncSimple>("Application.Fullscreen.Toggle", []()
            {
                IApplication::Get()->GetPrimaryWindow()->SetFullscreen(!IApplication::Get()->GetPrimaryWindow()->IsFullscreen());
            });

        PK_LOG_HEADER("----------RendererApplication.Ctor End----------");
    }

    RendererApplication::~RendererApplication()
    {
        Platform::SetInputHandler(nullptr);
        GetService<Sequencer>()->Release();
        GetService<AssetDatabase>()->UnloadAll();
        GetServices()->Clear();
        m_window = nullptr;
        m_RHIDriver = nullptr;
        PK_LOG_HEADER("----------RendererApplication.Dtor----------");
    }

    void RendererApplication::Execute()
    {
        auto sequencer = GetService<Sequencer>();

        while (m_isRunning)
        {
            Platform::PollEvents();

            if (m_window->IsMinimized())
            {
                Platform::WaitEvents();
                continue;
            }

            FrameContext ctx{};
            ctx.window = m_window.get();

            sequencer->NextRoot(FrameStep::Initialize(), &ctx);

            m_window->AcquireImage();

            sequencer->NextRoot(FrameStep::Update(), &ctx);
            sequencer->NextRoot(FrameStep::Render(), &ctx);

            m_window->PresentImage();

            sequencer->NextRoot(FrameStep::Finalize(), &ctx);

            RHI::GC();

            if (m_inactiveFrameInterval > 0 && !Platform::GetHasFocus())
            {
                Sleep(m_inactiveFrameInterval);
            }
        }
    }

    void RendererApplication::Close()
    {
        m_isRunning = false;
        PK_LOG_INFO("Application.Close Requested.");
    }
}

PK::IApplication* PK::CreateProjectApplication(const PK::CArguments& arguments)
{
    return new App::RendererApplication(arguments);
}

void PK::FreeProjectApplication(IApplication* application)
{
    delete application;
}
