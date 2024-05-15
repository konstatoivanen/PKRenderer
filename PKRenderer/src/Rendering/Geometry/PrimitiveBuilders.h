#pragma once
#include "Rendering/Objects/StaticMeshCollection.h"
#include "Rendering/Objects/StaticMeshAsset.h"
#include "Rendering/Objects/Mesh.h"
#include "Math/Types.h"

namespace PK::Rendering::Geometry
{
    Utilities::Ref<PK::Rendering::Objects::Mesh> CreateBoxMesh(const Math::float3& offset, const Math::float3& extents);

    Utilities::Ref<PK::Rendering::Objects::Mesh> CreateQuadMesh(const Math::float2& min, const Math::float2& max);

    PK::Rendering::Objects::StaticMeshAssetRef CreatePlaneVirtualMesh(PK::Rendering::Objects::StaticMeshCollection* baseMesh,
        const Math::float2& center,
        const Math::float2& extents,
        Math::uint2 resolution);

    PK::Rendering::Objects::StaticMeshAssetRef CreateSphereVirtualMesh(PK::Rendering::Objects::StaticMeshCollection* baseMesh,
        const Math::float3& offset,
        const float radius);
}
