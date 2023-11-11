#pragma once
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/MeshletMesh.h"
#include "Rendering/Objects/VirtualMesh.h"
#include "Math/Types.h"

namespace PK::Rendering::MeshUtilities
{
    Utilities::Ref<PK::Rendering::Objects::Mesh> CreateBoxMesh(const Math::float3& offset, const Math::float3& extents);
    
    Utilities::Ref<PK::Rendering::Objects::Mesh> CreateQuadMesh(const Math::float2& min, const Math::float2& max);
    
    Utilities::Ref<PK::Rendering::Objects::VirtualMesh> CreatePlaneVirtualMesh(Utilities::Ref<PK::Rendering::Objects::Mesh> baseMesh,
                                                                               Utilities::Ref<PK::Rendering::Objects::MeshletMesh> baseMeshletMesh,
                                                                               const Math::float2& center, 
                                                                               const Math::float2& extents, 
                                                                               Math::uint2 resolution);

    Utilities::Ref<PK::Rendering::Objects::VirtualMesh> CreateSphereVirtualMesh(Utilities::Ref<PK::Rendering::Objects::Mesh> baseMesh,
                                                                                Utilities::Ref<PK::Rendering::Objects::MeshletMesh> baseMeshletMesh,
                                                                                const Math::float3& offset, 
                                                                                const float radius);
}
