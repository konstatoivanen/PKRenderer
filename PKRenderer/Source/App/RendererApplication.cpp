#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/LoggerPrintf.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ControlFlow/RemoteProcessRunner.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/Input/InputSystem.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Window.h"
#include "Core/IApplication.h"
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
    RendererApplication::RendererApplication(const CArguments& arguments) :
        IApplication(arguments, "PK Renderer", CreateRef<LoggerPrintf>())
    {
        PK_LOG_TIMER_FUNC();
        PK_LOG_HEADER_SCOPE("----------RendererApplication.Ctor Begin----------");

        GetServices()->Create<HashCache>();

        auto remoteProcessRunner = GetServices()->Create<RemoteProcessRunner>();
        auto entityDb = GetServices()->Create<EntityDatabase>();
        auto sequencer = GetServices()->Create<Sequencer>();
        auto assetDatabase = GetServices()->Create<AssetDatabase>(sequencer);
        auto inputSystem = GetServices()->Create<InputSystem>(sequencer);
        Platform::SetInputHandler(inputSystem);

        auto config = assetDatabase->Load<BaseRendererConfig>("Content/Configs/BaseRenderer.cfg");
        auto keyConfig = assetDatabase->Load<InputKeyConfig>("Content/Configs/Input.keycfg");

        m_RHIDriver = RHI::CreateDriver(GetWorkingDirectory(), config->RHIDesc);

        auto windowDescriptor = config->WindowDesc;
        windowDescriptor.title = { GetName(), m_RHIDriver->GetDriverHeader() };
        m_window = CreateUnique<Window>(windowDescriptor);
        m_window->SetOnCloseCallback([this]() { Close(); });
        m_window->SetCursorLock(false, true);

        m_inactiveFrameInterval = config->InactiveFrameInterval;

        assetDatabase->LoadDirectory<ShaderAsset>("Content/Shaders/");

        auto time = GetServices()->Create<EngineTime>(sequencer, config->TimeScale);
        auto batcherMeshStatic = GetServices()->Create<BatcherMeshStatic>();
        auto renderPipelineScene = GetServices()->Create<RenderPipelineScene>(assetDatabase, entityDb, sequencer, batcherMeshStatic);

        auto engineViewUpdate = GetServices()->Create<EngineViewUpdate>(sequencer, entityDb);
        auto engineCommands = GetServices()->Create<EngineCommandInput>(sequencer, keyConfig);
        auto engineUpdateTransforms = GetServices()->Create<EngineUpdateTransforms>(entityDb);
        auto engineEntityCull = GetServices()->Create<EngineEntityCull>(entityDb);
        auto engineDrawGeometry = GetServices()->Create<EngineDrawGeometry>(entityDb, sequencer);
        auto engineGatherRayTracingGeometry = GetServices()->Create<EngineGatherRayTracingGeometry>(entityDb);
        auto engineScreenshot = GetServices()->Create<EngineScreenshot>();
        auto engineGUIRenderer = GetServices()->Create<EngineGUIRenderer>(assetDatabase, sequencer);
        auto engineProfiler = GetServices()->Create<EngineProfiler>(assetDatabase);

        auto engineFlyCamera = GetServices()->Create<EngineFlyCamera>(entityDb, keyConfig);
        auto engineDebug = GetServices()->Create<EngineDebug>(assetDatabase, entityDb, batcherMeshStatic->GetMeshStaticCollection());
        
        auto cvariableRegister = GetService<CVariableRegister>();

        sequencer->SetSteps(
            {
                {
                    sequencer->GetRoot(),
                    {
                        Sequencer::Step::Create<ApplicationStep::OpenFrame>(time),
                        Sequencer::Step::Create<ApplicationStep::UpdateInput, Window*>(inputSystem),
                        Sequencer::Step::Create<ApplicationStep::UpdateEngines>(engineFlyCamera),
                        Sequencer::Step::Create<ApplicationStep::UpdateEngines>(engineUpdateTransforms),
                        Sequencer::Step::Create<ApplicationStep::Render, Window*>(renderPipelineScene),
                        Sequencer::Step::Create<ApplicationStep::Render, Window*>(engineScreenshot),
                        Sequencer::Step::Create<ApplicationStep::CloseFrame>(time),
                        Sequencer::Step::Create<ApplicationStep::CloseFrame, Window*>(inputSystem),
                        Sequencer::Step::Create<CArgumentsConst>(cvariableRegister),
                        Sequencer::Step::Create<CArgumentConst>(cvariableRegister),
                        Sequencer::Step::Create<RemoteProcessCommand*>(remoteProcessRunner)
                    }
                },
                {
                    time,
                    {
                        Sequencer::Step::Create<TimeFrameInfo*>(engineViewUpdate),
                        Sequencer::Step::Create<TimeFramerateInfo*>(engineProfiler),
                    }
                },
                {
                    inputSystem,
                    {
                        Sequencer::Step::Create<InputState*>(engineCommands),
                        Sequencer::Step::Create<InputState*>(engineViewUpdate),
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
        GetService<AssetDatabase>()->Unload();
        GetServices()->Clear();
        m_window = nullptr;
        m_RHIDriver = nullptr;
        PK_LOG_HEADER("----------RendererApplication.Dtor----------");
    }

    void RendererApplication::Execute()
    {
        auto sequencer = GetService<Sequencer>();

        while (!m_window->IsClosing())
        {
            Platform::PollEvents();

            if (m_window->IsMinimized())
            {
                Platform::WaitEvents();
                continue;
            }

            if (m_inactiveFrameInterval > 0 && !Platform::GetHasFocus())
            {
                Sleep(m_inactiveFrameInterval);
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
