#include "PrecompiledHeader.h"
#include "Builders.h"
#include "Rendering/HashCache.h"
#include "ECS/Contextual/Implementers/MeshRenderableImplementer.h"
#include "ECS/Contextual/Implementers/LightImplementer.h"
#include "ECS/Contextual/EntityViews/LightSphereView.h"

namespace PK::ECS::Builders
{
	using namespace PK::Rendering;
	using namespace PK::Core::Services;
	using namespace EntityViews;
	using namespace Implementers;

    EGID BuildMeshRenderableEntity(EntityDatabase* entityDb,
							  Mesh* mesh, 
						      std::initializer_list<Material*> materials, 
							  const float3& position, 
							  const float3& rotation, 
							  float size, 
							  RenderableFlags flags)
    {
		auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
		auto implementer = entityDb->ReserveImplementer<MeshRenderableImplementer>();
		BuildTransformView(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE * size, mesh->GetLocalBounds());
		BuildBaseRenderableView(entityDb, implementer, egid, flags | RenderableFlags::Mesh);
		BuildMeshRenderableView(entityDb, implementer, egid, mesh, materials);
		return egid;
    }

	EGID BuildLightRenderableEntity(EntityDatabase* entityDb, 
								AssetDatabase* assetDatabase, 
								const float3& position, 
								const float3& rotation,
								LightType type, 
								Cookie cookie, 
								const color& color, 
								float angle, 
								float radius, 
								bool castShadows)
	{
		auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
		auto implementer = entityDb->ReserveImplementer<LightImplementer>();
		BuildTransformView(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE, {});
		BuildLightRenderableViews(entityDb, implementer, egid, type, cookie, color, angle, radius, castShadows);
		auto lightSphereView = entityDb->ReserveEntityView<LightSphereView>(egid);
		return egid;
	}

	EGID BuildLightSphereRenderableEntity(EntityDatabase* entityDb, 
										AssetDatabase* assetDatabase, 
										const float3& position, 
										LightType type,
										Cookie cookie, 
										const color& color, 
										bool castShadows)
	{
		auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
		auto implementer = entityDb->ReserveImplementer<LightImplementer>();
		BuildTransformView(entityDb, implementer, egid, position, PK_FLOAT3_ZERO, PK_FLOAT3_ONE, {});
		BuildLightRenderableViews(entityDb, implementer, egid, type, cookie, color, 90.0f, -1.0f, castShadows);
		auto lightSphereView = entityDb->ReserveEntityView<LightSphereView>(egid);

		const auto sphereRadius = 0.2f;
		const auto sphereTranslucency = 0.1f;
		auto hdrColor = implementer->color * sphereTranslucency * (1.0f / (sphereRadius * sphereRadius));

		auto mesh = assetDatabase->Find<Mesh>("Primitive_Sphere");
		auto shader = assetDatabase->Find<Shader>("SH_WS_Unlit_Color");
		auto material = assetDatabase->RegisterProcedural("M_Point_Light_" + std::to_string(egid.entityID()), CreateRef<Material>(shader, nullptr));
		material->Set<float4>(HashCache::Get()->_Color, hdrColor);

		auto meshEgid = BuildMeshRenderableEntity(entityDb, mesh, { material }, position, PK_FLOAT3_ZERO, sphereRadius, RenderableFlags::Cullable);
		lightSphereView->transformMesh = entityDb->Query<EntityViews::TransformView>(meshEgid)->transform;
		lightSphereView->transformLight = implementer;
		return egid;
	}
}