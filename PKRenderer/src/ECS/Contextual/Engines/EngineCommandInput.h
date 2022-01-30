#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/Ref.h"
#include "Core/Services/Time.h"
#include "Core/Services/Input.h"
#include "Core/Services/AssetDataBase.h"
#include "Core/CommandConfig.h"
#include "Core/UpdateStep.h"
#include "ECS/EntityDatabase.h"

namespace PK::ECS::Engines
{
	enum class CommandArgument : char
	{
		Query,
		Reload,
		Exit,
		Application,
		Contextual,
		VSync,
		Assets,
		StringParameter,
		Variants,
		Uniforms,
		GPUMemory,
		TypeShader,
		TypeMesh,
		TypeTexture,
		TypeMaterial,
		TypeTime,
		TypeAppConfig
	};

	class ConsoleCommand : public std::vector<std::string>
	{
	};

	class EngineCommandInput : public Core::Services::IService, public Core::Services::IStep<Core::Services::Input>
	{
		public:
			const static std::unordered_map<std::string, CommandArgument> ArgumentMap;

			EngineCommandInput(Core::Services::AssetDatabase* assetDatabase,
							   Core::Services::Sequencer* sequencer, 
							   Core::Services::Time* time, 
							   EntityDatabase* entityDb, 
							   Core::CommandConfig* commandBindings);

			void Step(Core::Services::Input* input) override final;

		private:
			void ApplicationExit(const ConsoleCommand& arguments);
			void ApplicationContextual(const ConsoleCommand& arguments);
			void ApplicationSetVSync(const ConsoleCommand& arguments);
			void QueryShaderVariants(const ConsoleCommand& arguments);
			void QueryShaderUniforms(const ConsoleCommand& arguments);
			void QueryGPUMemory(const ConsoleCommand& arguments);
			void ReloadTime(const ConsoleCommand& arguments);
			void ReloadAppConfig(const ConsoleCommand& arguments);
			void ReloadShaders(const ConsoleCommand& arguments);
			void ReloadMaterials(const ConsoleCommand& arguments);
			void ReloadTextures(const ConsoleCommand& arguments);
			void ReloadMeshes(const ConsoleCommand& arguments);
			void QueryLoadedShaders(const ConsoleCommand& arguments);
			void QueryLoadedMaterials(const ConsoleCommand& arguments);
			void QueryLoadedTextures(const ConsoleCommand& arguments);
			void QueryLoadedMeshes(const ConsoleCommand& arguments);
			void QueryLoadedAssets(const ConsoleCommand& arguments);
			void ProcessCommand(const std::string& command);

			std::map<std::vector<CommandArgument>, std::function<void(const ConsoleCommand&)>> m_commands;
			Core::CommandConfig* m_commandBindings = nullptr;
			EntityDatabase* m_entityDb = nullptr;
			Core::Services::AssetDatabase* m_assetDatabase = nullptr;
			Core::Services::Sequencer* m_sequencer = nullptr;
			Core::Services::Time* m_time = nullptr;
	};
}