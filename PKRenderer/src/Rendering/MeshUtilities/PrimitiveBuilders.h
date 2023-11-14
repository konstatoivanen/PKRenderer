#pragma once
#include "Rendering/Objects/StaticSceneMesh.h"
#include "Rendering/Objects/VirtualStaticMesh.h"
#include "Rendering/Objects/Mesh.h"
#include "Math/Types.h"

namespace PK::Rendering::MeshUtilities
{
    Utilities::Ref<PK::Rendering::Objects::Mesh> CreateBoxMesh(const Math::float3& offset, const Math::float3& extents);
    
    Utilities::Ref<PK::Rendering::Objects::Mesh> CreateQuadMesh(const Math::float2& min, const Math::float2& max);
    
    PK::Rendering::Objects::VirtualStaticMeshRef CreatePlaneVirtualMesh(PK::Rendering::Objects::StaticSceneMesh* baseMesh,
                                                                        const Math::float2& center, 
                                                                        const Math::float2& extents, 
                                                                        Math::uint2 resolution);

    PK::Rendering::Objects::VirtualStaticMeshRef CreateSphereVirtualMesh(PK::Rendering::Objects::StaticSceneMesh* baseMesh,
                                                                         const Math::float3& offset, 
                                                                         const float radius);
}
