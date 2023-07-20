#pragma once
#include "ECS/EntityDatabase.h"
#include "Math/Types.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/VirtualMesh.h"
#include "Rendering/Objects/Material.h"
#include "ECS/EntityViews/TransformView.h"
#include "ECS/EntityViews/BaseRenderableView.h"
#include "ECS/EntityViews/MeshRenderableView.h"
#include "ECS/EntityViews/LightRenderableView.h"

namespace PK::ECS::Builders
{
	template<typename T>
	void BuildTransformView(EntityDatabase* entityDb, 
							T* implementer, 
							const EGID& egid, 
							const Math::float3& position, 
							const Math::float3& rotation,
							const Math::float3& scale, 
							const Math::BoundingBox& localBounds)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, 
			&EntityViews::TransformView::bounds,
			&EntityViews::TransformView::transform);
		implementer->localAABB = localBounds;
		implementer->position = position;
		implementer->rotation = glm::quat(rotation * Math::PK_FLOAT_DEG2RAD);
		implementer->scale = scale;
	}

	template<typename T>
	void BuildBaseRenderableView(EntityDatabase* entityDb, 
								 T* implementer, 
								 const EGID& egid, 
								 Rendering::Structs::RenderableFlags flags)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, 
			&EntityViews::BaseRenderableView::renderable,
			&EntityViews::BaseRenderableView::bounds);
		implementer->flags = flags;
	}

	template<typename T>
	void BuildMeshRenderableView(EntityDatabase* entityDb, 
								 T* implementer, 
								 const EGID& egid, 
								 Rendering::Objects::Mesh* mesh,
								 const std::initializer_list<Rendering::Objects::MaterialTarget>& materials)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, 
			&EntityViews::MeshRenderableView::materials, 
			&EntityViews::MeshRenderableView::mesh, 
			&EntityViews::MeshRenderableView::transform);
		implementer->materials = materials;
		implementer->sharedMesh = mesh;
	}

	template<typename T>
	void BuildLightRenderableView(EntityDatabase* entityDb, 
								  T* implementer, 
								  const EGID& egid, 
								  Rendering::Structs::LightType type,
								  Rendering::Structs::Cookie cookie,
								  const Math::color& color,
								  float radius, 
								  float angle)
	{
		auto view = entityDb->ReserveEntityView(implementer, egid, 
			&EntityViews::LightRenderableView::transform, 
			&EntityViews::LightRenderableView::bounds, 
			&EntityViews::LightRenderableView::light, 
			&EntityViews::LightRenderableView::lightFrameInfo, 
			&EntityViews::LightRenderableView::renderable);
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
								   Rendering::Structs::LightType type,
								   Rendering::Structs::Cookie cookie,
								   const Math::color& color,
								   float angle, 
								   float radius,
								   bool castShadows)
	{
		const auto intensityThreshold = 0.2f;
		auto lightColor = glm::exp(color);
		auto gammaColor = glm::pow(Math::float3(lightColor.rgb), Math::float3(1.0f / 2.2f));
		auto intensity = glm::compMax(gammaColor);
		auto autoRadius = radius < 0.0f ? intensity / intensityThreshold : radius;
		auto flags = Rendering::Structs::RenderableFlags::Light;

		if (castShadows)
		{
			flags = flags | Rendering::Structs::RenderableFlags::CastShadows;
		}

		switch (type)
		{
			case Rendering::Structs::LightType::Directional:
				implementer->localAABB = Math::BoundingBox::CenterExtents(Math::PK_FLOAT3_ZERO, Math::PK_FLOAT3_ONE);
				break;
			case Rendering::Structs::LightType::Point:
				implementer->localAABB = Math::BoundingBox::CenterExtents(Math::PK_FLOAT3_ZERO, Math::PK_FLOAT3_ONE * autoRadius);
				flags = flags | Rendering::Structs::RenderableFlags::Cullable;
				break;
			case Rendering::Structs::LightType::Spot:
				auto a = autoRadius * glm::tan(angle * 0.5f * Math::PK_FLOAT_DEG2RAD);
				implementer->localAABB = Math::BoundingBox::CenterExtents({ 0.0f, 0.0f, autoRadius * 0.5f }, { a, a, autoRadius * 0.5f });
				flags = flags | Rendering::Structs::RenderableFlags::Cullable;
				break;
		}

		BuildBaseRenderableView(entityDb, implementer, egid, flags);
		BuildLightRenderableView(entityDb, implementer, egid, type, cookie, lightColor, autoRadius, angle);
	}

	EGID BuildMeshRenderableEntity(EntityDatabase* entityDb,
								   Rendering::Objects::Mesh* mesh,
								   const std::initializer_list<Rendering::Objects::MaterialTarget>& materials,
								   const Math::float3& position,
								   const Math::float3& rotation,
								   float size = 1.0f, 
								   Rendering::Structs::RenderableFlags flags = Rendering::Structs::RenderableFlags::DefaultMesh);

	EGID BuildMeshRenderableEntity(EntityDatabase* entityDb, 
								   Rendering::Objects::VirtualMesh* mesh,
								   const std::initializer_list<Rendering::Objects::MaterialTarget>& materials,
								   const Math::float3& position,
								   const Math::float3& rotation,
								   float size = 1.0f, 
								   Rendering::Structs::RenderableFlags flags = Rendering::Structs::RenderableFlags::DefaultMesh);

	EGID BuildLightRenderableEntity(EntityDatabase* entityDb,
								Core::Services::AssetDatabase* assetDatabase,
								const Math::float3& position,
								const Math::float3& rotation,
								Rendering::Structs::LightType type,
								Rendering::Structs::Cookie cookie,
								const Math::color& color,
								float angle,
								float radius,
								bool castShadows);

	EGID BuildLightSphereRenderableEntity(EntityDatabase* entityDb,
										Core::Services::AssetDatabase* assetDatabase,
										const Math::float3& position,
										Rendering::Structs::LightType type,
										Rendering::Structs::Cookie cookie,
										const Math::color& color,
										bool castShadows);

}