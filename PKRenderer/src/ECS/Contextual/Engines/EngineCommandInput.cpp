#include "PrecompiledHeader.h"
#include "EngineCommandInput.h"
#include "Core/Application.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/Objects/Texture.h"

namespace PK::ECS::Engines
{
    const std::unordered_map<std::string, CommandArgument> EngineCommandInput::ArgumentMap =
    {
        {std::string("query"),      CommandArgument::Query},
        {std::string("reload"),     CommandArgument::Reload},
        {std::string("exit"),       CommandArgument::Exit},
        {std::string("application"),CommandArgument::Application},
        {std::string("contextual"), CommandArgument::Contextual},
        {std::string("vsync"),      CommandArgument::VSync},
        {std::string("assets"),     CommandArgument::Assets},
        {std::string("variants"),   CommandArgument::Variants},
        {std::string("uniforms"),   CommandArgument::Uniforms},
        {std::string("gpu_memory"), CommandArgument::GPUMemory},
        {std::string("shader"),     CommandArgument::TypeShader},
        {std::string("mesh"),       CommandArgument::TypeMesh},
        {std::string("texture"),    CommandArgument::TypeTexture},
        {std::string("material"),   CommandArgument::TypeMaterial},
        {std::string("time"),       CommandArgument::TypeTime},
        {std::string("appconfig"),  CommandArgument::TypeAppConfig},
        {std::string("remoteExecute"), CommandArgument::RemoteExecute},
    };

    void EngineCommandInput::ApplicationExit(const ConsoleCommand& arguments) { Application::Get().Close(); }

    void EngineCommandInput::ApplicationContextual(const ConsoleCommand& arguments)
    {
        ConsoleCommandToken token = { arguments.at(2), false };
        m_sequencer->Next(this, &token, 0);

        if (!token.isConsumed)
        {
            PK_LOG_INFO("Command not consumed!");
        }
    }

    void EngineCommandInput::ApplicationSetVSync(const ConsoleCommand& arguments)
    {
        const auto& str = arguments.at(2);
        if (str == "true") Application::GetPrimaryWindow()->SetVSync(true);
        if (str == "false") Application::GetPrimaryWindow()->SetVSync(false);
        if (str == "toggle") Application::GetPrimaryWindow()->SetVSync(!Application::GetPrimaryWindow()->IsVSync());
        PK_LOG_INFO("VSync: %s", (Application::GetPrimaryWindow()->IsVSync() ? "Enabled" : "Disabled"));
    }

    void EngineCommandInput::QueryShaderVariants(const ConsoleCommand& arguments)
    {
        auto shader = m_assetDatabase->TryFind<Shader>(arguments[2].c_str());

        if (shader != nullptr)
        {
            shader->ListVariants();
        }
        else
        {
            PK_LOG_NEWLINE();
            PK_LOG_WARNING("Could not find shader with keyword: %s", arguments[2].c_str());
            PK_LOG_NEWLINE();
        }
    }

    void EngineCommandInput::QueryShaderUniforms(const ConsoleCommand& arguments)
    {
        auto shader = m_assetDatabase->TryFind<Shader>(arguments[2].c_str());

        if (shader != nullptr)
        {
            shader->ListProperties((uint32_t)std::stoi(arguments[3].c_str()));
        }
        else
        {
            PK_LOG_NEWLINE();
            PK_LOG_WARNING("Could not find shader with keyword: %s", arguments[2].c_str());
            PK_LOG_NEWLINE();
        }
    }

    void EngineCommandInput::QueryGPUMemory(const ConsoleCommand& arguments)
    {
        PK_LOG_INFO("GPU Memory usage in kb: %i", Rendering::GraphicsAPI::GetMemoryUsageKB());
    }

    void EngineCommandInput::ReloadTime(const ConsoleCommand& arguments)
    {
        Application::GetService<Time>()->Reset();
        PK_LOG_INFO("Time Reset.");
    }

    void EngineCommandInput::ReloadAppConfig(const ConsoleCommand& arguments)
    {
        m_assetDatabase->ReloadDirectory<ApplicationConfig>(arguments[2].c_str());
        PK_LOG_INFO("Reimported application configs in folder: %s", arguments[2].c_str());
    }

    void EngineCommandInput::ReloadShaders(const ConsoleCommand& arguments)
    {
        m_assetDatabase->ReloadDirectory<Shader>(arguments[2].c_str());
        PK_LOG_INFO("Reimported shaders in folder: %s", arguments[2].c_str());
    }

    void EngineCommandInput::ReloadMaterials(const ConsoleCommand& arguments)
    {
       //@TODO m_assetDatabase->ReloadDirectory<Material>(arguments[2].c_str());
        PK_LOG_INFO("Reimported materials in folder: %s", arguments[2].c_str());
    }

    void EngineCommandInput::ReloadTextures(const ConsoleCommand& arguments)
    {
        m_assetDatabase->ReloadDirectory<Texture>(arguments[2].c_str());
        PK_LOG_INFO("Reimported textures in folder: %s", arguments[2].c_str());
    }

    void EngineCommandInput::ReloadMeshes(const ConsoleCommand& arguments)
    {
        m_assetDatabase->ReloadDirectory<Mesh>(arguments[2].c_str());
        PK_LOG_INFO("Reimported meshes in folder: %s", arguments[2].c_str());
    }


    void EngineCommandInput::QueryLoadedShaders(const ConsoleCommand& arguments) { m_assetDatabase->ListAssetsOfType<Shader>(); }
    void EngineCommandInput::QueryLoadedMaterials(const ConsoleCommand& arguments) {}//@TODO m_assetDatabase->ListAssetsOfType<Material>(); }
    void EngineCommandInput::QueryLoadedTextures(const ConsoleCommand& arguments) { m_assetDatabase->ListAssetsOfType<Texture>(); }
    void EngineCommandInput::QueryLoadedMeshes(const ConsoleCommand& arguments) { m_assetDatabase->ListAssetsOfType<Mesh>(); }
    void EngineCommandInput::QueryLoadedAssets(const ConsoleCommand& arguments) { m_assetDatabase->ListAssets(); }

    void EngineCommandInput::RemoteExecute(const ConsoleCommand& arguments)
    {
        PK_LOG_INFO("Remote Execution: %s", arguments[1].c_str());
        system(arguments[1].c_str());
    }

    void EngineCommandInput::ProcessCommand(const std::string& command)
    {
        std::string argument;
        std::istringstream iss(command);
        std::vector<CommandArgument> commandArguments;
        ConsoleCommand arguments;

        while (iss >> argument)
        {
            arguments.push_back(argument);

            if (ArgumentMap.count(argument))
            {
                commandArguments.push_back(ArgumentMap.at(argument));
            }
            else
            {
                commandArguments.push_back(CommandArgument::StringParameter);
            }
        }

        if (commandArguments.empty())
        {
            return;
        }

        if (m_commands.count(commandArguments))
        {
            m_commands.at(commandArguments)(arguments);
        }
        else
        {
            PK_LOG_NEWLINE();
            PK_LOG_WARNING("Unknown command!");
            PK_LOG_NEWLINE();
        }
    }

    EngineCommandInput::EngineCommandInput(AssetDatabase* assetDatabase, Sequencer* sequencer, Time* time, EntityDatabase* entityDb, CommandConfig* commandBindings)
    {
        m_entityDb = entityDb;
        m_assetDatabase = assetDatabase;
        m_time = time;
        m_sequencer = sequencer;
        m_commandBindings = commandBindings;

        m_commands[{CommandArgument::Application, CommandArgument::Exit }] = PK_BIND_FUNCTION(ApplicationExit);
        m_commands[{CommandArgument::Application, CommandArgument::Contextual, CommandArgument::StringParameter }] = PK_BIND_FUNCTION(ApplicationContextual);
        m_commands[{CommandArgument::Application, CommandArgument::VSync, CommandArgument::StringParameter }] = PK_BIND_FUNCTION(ApplicationSetVSync);
        m_commands[{CommandArgument::Query, CommandArgument::TypeShader, CommandArgument::StringParameter, CommandArgument::Variants}] = PK_BIND_FUNCTION(QueryShaderVariants);
        m_commands[{CommandArgument::Query, CommandArgument::TypeShader, CommandArgument::StringParameter, CommandArgument::Uniforms, CommandArgument::StringParameter}] = PK_BIND_FUNCTION(QueryShaderUniforms);
        m_commands[{CommandArgument::Query, CommandArgument::GPUMemory}] = PK_BIND_FUNCTION(QueryGPUMemory);
        m_commands[{CommandArgument::Query, CommandArgument::Assets, CommandArgument::TypeShader}] = PK_BIND_FUNCTION(QueryLoadedShaders);
        m_commands[{CommandArgument::Query, CommandArgument::Assets, CommandArgument::TypeMaterial}] = PK_BIND_FUNCTION(QueryLoadedMaterials);
        m_commands[{CommandArgument::Query, CommandArgument::Assets, CommandArgument::TypeMesh}] = PK_BIND_FUNCTION(QueryLoadedMeshes);
        m_commands[{CommandArgument::Query, CommandArgument::Assets, CommandArgument::TypeTexture}] = PK_BIND_FUNCTION(QueryLoadedTextures);
        m_commands[{CommandArgument::Query, CommandArgument::Assets}] = PK_BIND_FUNCTION(QueryLoadedAssets);
        m_commands[{CommandArgument::Reload, CommandArgument::TypeShader, CommandArgument::StringParameter}] = PK_BIND_FUNCTION(ReloadShaders);
        m_commands[{CommandArgument::Reload, CommandArgument::TypeMesh, CommandArgument::StringParameter}] = PK_BIND_FUNCTION(ReloadMeshes);
        m_commands[{CommandArgument::Reload, CommandArgument::TypeMaterial, CommandArgument::StringParameter}] = PK_BIND_FUNCTION(ReloadMaterials);
        m_commands[{CommandArgument::Reload, CommandArgument::TypeTexture, CommandArgument::StringParameter}] = PK_BIND_FUNCTION(ReloadTextures);
        m_commands[{CommandArgument::Reload, CommandArgument::TypeAppConfig, CommandArgument::StringParameter}] = PK_BIND_FUNCTION(ReloadAppConfig);
        m_commands[{CommandArgument::Reload, CommandArgument::TypeTime}] = PK_BIND_FUNCTION(ReloadTime);
        m_commands[{CommandArgument::RemoteExecute, CommandArgument::StringParameter}] = PK_BIND_FUNCTION(RemoteExecute);
    }
    
    void EngineCommandInput::Step(Input* input)
    {
        if (m_commandBindings != nullptr)
        {
            for (auto& binding : m_commandBindings->Commands.value)
            {
                if (input->GetKeyDown(binding.keycode))
                {
                    ProcessCommand(binding.command);
                }
            }
        }

        if (!input->GetKeyDown(KeyCode::TAB))
        {
            return;
        }

        PK_LOG_NEWLINE();
        auto flag = PK::Core::Services::Debug::PK_LOG_LVL_INFO;
        PK::Core::Services::Debug::Logger::Get()->Log(flag, (int)PK::Core::Services::Debug::ConsoleColor::LOG_INPUT, "Awaiting command input...");

        std::string command;
        std::getline(std::cin, command);
        ProcessCommand(command);
    }
}