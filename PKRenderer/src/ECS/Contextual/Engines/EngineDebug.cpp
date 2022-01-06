#include "PrecompiledHeader.h"
#include "EngineDebug.h"
#include "ECS/Contextual/Builders/Builders.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsColor.h"

namespace PK::ECS::Engines
{
	using namespace PK::Rendering::Objects;
	using namespace PK::Rendering::Structs;
	using namespace PK::Math;

	EngineDebug::EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, const ApplicationConfig* config)
	{
		m_entityDb = entityDb;
		m_assetDatabase = assetDatabase;

		//auto buildingsMesh = assetDatabase->Load<Mesh>("res/models/Buildings.mdl");
		//auto spiralMesh = assetDatabase->Load<Mesh>("res/models/Spiral.mdl");
		//auto treeMesh = assetDatabase->Load<Mesh>("res/models/Tree.mdl");
		
		auto columnMesh = assetDatabase->Load<Mesh>("res/models/MDL_Columns.pkmesh");
		auto clothMesh = assetDatabase->Load<Mesh>("res/models/MDL_Cloth.pkmesh");
		auto sphereMesh = assetDatabase->RegisterProcedural<Mesh>("Primitive_Sphere", Rendering::MeshUtility::GetSphere(PK_FLOAT3_ZERO, 1.0f));
		auto planeMesh = assetDatabase->RegisterProcedural<Mesh>("Primitive_Plane16x16", Rendering::MeshUtility::GetPlane(PK_FLOAT2_ZERO, PK_FLOAT2_ONE, { 16, 16 }));
		auto oceanMesh = assetDatabase->RegisterProcedural<Mesh>("Primitive_Plane128x128", Rendering::MeshUtility::GetPlane(PK_FLOAT2_ZERO, PK_FLOAT2_ONE * 5.0f, { 128, 128 }));

		auto materialMetal = assetDatabase->Load<Material>("res/materials/M_Metal_Panel.material");
		auto materialCloth = assetDatabase->Load<Material>("res/materials/M_Cloth.material");
		auto materialGravel = assetDatabase->Load<Material>("res/materials/M_Gravel.material");
		auto materialGround = assetDatabase->Load<Material>("res/materials/M_Ground.material");
		auto materialSand = assetDatabase->Load<Material>("res/materials/M_Sand.material");
		auto materialWood = assetDatabase->Load<Material>("res/materials/M_Wood_Floor.material");
		auto materialAsphalt = assetDatabase->Load<Material>("res/materials/M_Asphalt.material");
		auto materialWater = assetDatabase->Load<Material>("res/materials/M_Water.material");

		auto minpos = float3(-70, -5, -70);
		auto maxpos = float3(70, 0, 70);

		srand(config->RandomSeed);

		Builders::BuildMeshRenderableEntity(m_entityDb, planeMesh, { materialSand }, { 0, -5, 0 }, { 90, 0, 0 }, 80.0f);
		//Builders::BuildMeshRenderableEntity(m_entityDb, columnMesh, { materialAsphalt }, { -20, 5, -20 }, PK_FLOAT3_ZERO, 3.0f);

		//CreateMeshRenderable(entityDb, float3(0, -5, 0), { 0, 0, 0 }, 1.0f, buildingsMesh, materialAsphalt);

		//CreateMeshRenderable(entityDb, float3(-25, -7.5f, 0), { 0, 90, 0 }, 1.0f, spiralMesh, materialAsphalt);

		//CreateMeshRenderable(entityDb, float3( 30, 0, 24), { 0, 90, 0 }, 2.0f, clothMesh, materialCloth);

		//CreateMeshRenderable(entityDb, float3( 55, 7, -15), { 90, 0, 0 }, 3.0f, oceanMesh, materialWater, false);

		//CreateMeshRenderable(entityDb, float3( -35, -5, -30), { 0, 0, 0 }, 2.0f, treeMesh, materialAsphalt, true);

		for (auto i = 0; i < 320; ++i)
		{
			Builders::BuildMeshRenderableEntity(m_entityDb, sphereMesh, { materialMetal }, Functions::RandomRangeFloat3(minpos, maxpos), Functions::RandomEuler());
		}

		for (auto i = 0; i < 320; ++i)
		{
			Builders::BuildMeshRenderableEntity(m_entityDb, sphereMesh, { materialGravel }, Functions::RandomRangeFloat3(minpos, maxpos), Functions::RandomEuler());
		}

		for (uint i = 0; i < config->LightCount; ++i)
		{
			Builders::BuildLightSphereRenderableEntity(
				m_entityDb,
				m_assetDatabase,
				Functions::RandomRangeFloat3(minpos, maxpos),
				i % 2 == 0 ? LightType::Spot : LightType::Point,
				Cookie::Circle0,
				Functions::HueToRGB(Functions::RandomRangeFloat(0.0f, 1.0f)) * Functions::RandomRangeFloat(6.0f, 7.0f),
				true);
		}

		auto color = Functions::HexToRGB(0xFFA575FF) * 2.0f; // 0x6D563DFF //0x66D1FFFF //0xF78B3DFF
		//Builders::BuildLightRenderableEntity(m_entityDb, m_assetDatabase, PK_FLOAT3_ZERO, { 25, -35, 0 }, LightType::Directional, Cookie::Circle0, color, 90.0f, 50.0f, true);
	}

	void EngineDebug::Step(int condition)
	{
		/*
		auto lights = m_entityDb->Query<EntityViews::LightSphere>((int)ENTITY_GROUPS::ACTIVE);
		auto time = Application::GetService<Time>()->GetTime();

		for (auto i = 0; i < lights.count; ++i)
		{
			// auto ypos = sin(time * 2 + ((float)i * 4 / lights.count));
			auto rotation = glm::quat(float3(0, time + float(i), 0));
			lights[i].transformLight->rotation = rotation;
			lights[i].transformMesh->rotation = rotation;
			//lights[i].transformLight->position.y = ypos;
			//lights[i].transformMesh->position.y = ypos;
		}

		return;

		auto meshes = m_entityDb->Query<EntityViews::MeshRenderable>((int)ENTITY_GROUPS::ACTIVE);

		for (auto i = 0; i < meshes.count; ++i)
		{
			meshes[i].transform->position.y = sin(time + (10 * (float)i / meshes.count)) * 10;
		}
		*/
	}
}