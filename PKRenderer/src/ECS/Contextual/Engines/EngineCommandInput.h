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
	using namespace PK::Utilities;
	using namespace PK::Rendering::Objects;

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

	class EngineCommandInput : public IService, public IStep<Input>
	{
		public:
			const static std::unordered_map<std::string, CommandArgument> ArgumentMap;

			EngineCommandInput(AssetDatabase* assetDatabase, Sequencer* sequencer, Time* time, EntityDatabase* entityDb, CommandConfig* commandBindings);
			void Step(Input* input) override final;

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
			CommandConfig* m_commandBindings = nullptr;
			EntityDatabase* m_entityDb = nullptr;
			AssetDatabase* m_assetDatabase = nullptr;
			Sequencer* m_sequencer = nullptr;
			Time* m_time = nullptr;
	};
}