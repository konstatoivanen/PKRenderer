#pragma once
#include "ECS/EntityDatabase.h"
#include "Math/Types.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/Material.h"
#include "ECS/Contextual/EntityViews/TransformView.h"
#include "ECS/Contextual/EntityViews/BaseRenderableView.h"
#include "ECS/Contextual/EntityViews/MeshRenderableView.h"
#include "ECS/Contextual/EntityViews/LightRenderableView.h"

namespace PK::ECS::Builders
{
	using namespace PK::Math;
	using namespace PK::Rendering::Objects;
	using namespace PK::Core::Services;
	using namespace EntityViews;

	template<typename T>
	void BuildTransformView(EntityDatabase* entityDb, 
							T* implementer, 
							const EGID& egid, 
							const float3& position, 
							const float3& rotation,
							const float3& scale, 
							const BoundingBox& localBounds)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, &TransformView::bounds, &TransformView::transform);
		implementer->localAABB = localBounds;
		implementer->position = position;
		implementer->rotation = glm::quat(rotation * PK_FLOAT_DEG2RAD);
		implementer->scale = scale;
	}

	template<typename T>
	void BuildBaseRenderableView(EntityDatabase* entityDb, 
								 T* implementer, 
								 const EGID& egid, 
								 RenderableFlags flags)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, &BaseRenderableView::renderable, &BaseRenderableView::bounds);
		implementer->flags = flags;
	}

	template<typename T>
	void BuildMeshRenderableView(EntityDatabase* entityDb, 
								 T* implementer, 
								 const EGID& egid, 
								 Mesh* mesh, 
								 const std::initializer_list<MaterialTarget>& materials)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, &MeshRenderableView::materials, &MeshRenderableView::mesh, &MeshRenderableView::transform);
		implementer->materials = materials;
		implementer->sharedMesh = mesh;
	}

	template<typename T>
	void BuildLightRenderableView(EntityDatabase* entityDb, 
								  T* implementer, 
								  const EGID& egid, 
								  LightType type, 
								  Cookie cookie, 
								  const color& color, 
								  float radius, 
								  float angle)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, 
			&LightRenderableView::transform, 
			&LightRenderableView::bounds, 
			&LightRenderableView::light, 
			&LightRenderableView::lightFrameInfo, 
			&LightRenderableView::renderable);
		implementer->color = color;
		implementer->radius = radius;
		implementer->angle = angle;
		implementer->cookie = cookie;
		implementer->type = type;
	}

	template<typename T>
	void BuildLightRenderableViews(EntityDatabase* entityDb, 
								   T* implementer, 
								   const EGID& egid, 
								   LightType type,
								   Cookie cookie, 
								   const color& color, 
								   float angle, 
								   float radius,
								   bool castShadows)
	{
		const auto intensityThreshold = 0.2f;
		auto lightColor = glm::exp(color);
		auto gammaColor = glm::pow(float3(lightColor.rgb), float3(1.0f / 2.2f));
		auto intensity = glm::compMax(gammaColor);
		auto autoRadius = radius < 0.0f ? intensity / intensityThreshold : radius;
		auto flags = RenderableFlags::Light;

		if (castShadows)
		{
			flags = flags | RenderableFlags::CastShadows;
		}

		switch (type)
		{
			case LightType::Directional:
				implementer->localAABB = BoundingBox::CenterExtents(PK_FLOAT3_ZERO, PK_FLOAT3_ONE);
				break;
			case LightType::Point:
				implementer->localAABB = BoundingBox::CenterExtents(PK_FLOAT3_ZERO, PK_FLOAT3_ONE * autoRadius);
				flags = flags | RenderableFlags::Cullable;
				break;
			case LightType::Spot:
				auto a = autoRadius * glm::tan(angle * 0.5f * PK_FLOAT_DEG2RAD);
				implementer->localAABB = BoundingBox::CenterExtents({ 0.0f, 0.0f, autoRadius * 0.5f }, { a, a, autoRadius * 0.5f });
				flags = flags | RenderableFlags::Cullable;
				break;
		}

		BuildBaseRenderableView(entityDb, implementer, egid, flags);
		BuildLightRenderableView(entityDb, implementer, egid, type, cookie, lightColor, autoRadius, angle);
	}

	EGID BuildMeshRenderableEntity(EntityDatabase* entityDb, 
								   Mesh* mesh, 
								   const std::initializer_list<MaterialTarget>& materials,
								   const float3& position, 
								   const float3& rotation, 
								   float size = 1.0f, 
								   RenderableFlags flags = RenderableFlags::DefaultMesh);

	EGID BuildLightRenderableEntity(EntityDatabase* entityDb,
								AssetDatabase* assetDatabase,
								const float3& position,
								const float3& rotation,
								LightType type,
								Cookie cookie,
								const color& color,
								float angle,
								float radius,
								bool castShadows);

	EGID BuildLightSphereRenderableEntity(EntityDatabase* entityDb,
										AssetDatabase* assetDatabase,
										const float3& position,
										LightType type,
										Cookie cookie,
										const color& color,
										bool castShadows);

}