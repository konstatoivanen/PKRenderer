#include "PrecompiledHeader.h"
#include "MeshUtility.h"
#include "Rendering/Structs/StructsCommon.h"
#include <mikktspace/mikktspace.h>

namespace PK::Rendering::MeshUtility
{
    using namespace Utilities;

    namespace MikktsInterface0
    {
        struct PKMeshData
        {
            const float* vertices = nullptr;
            const float* normals = nullptr;
            float* tangents = nullptr;
            const float* texcoords = nullptr;
            const unsigned int* indices = nullptr;
            unsigned int vcount = 0;
            unsigned int icount = 0;
        };

        // Returns the number of faces (triangles/quads) on the mesh to be processed.
        int GetNumFaces(const SMikkTSpaceContext* pContext)
        {
            return reinterpret_cast<PKMeshData*>(pContext->m_pUserData)->icount / 3;
        }

        // Returns the number of vertices on face number iFace
        // iFace is a number in the range {0, 1, ..., getNumFaces()-1}
        int GetNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
        {
            return 3;
        }

        // returns the position/normal/texcoord of the referenced face of vertex number iVert.
        // iVert is in the range {0,1,2} for triangles and {0,1,2,3} for quads.
        void GetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 + iVert];
            fvPosOut[0] = meshData->vertices[baseIndex * 3 + 0];
            fvPosOut[1] = meshData->vertices[baseIndex * 3 + 1];
            fvPosOut[2] = meshData->vertices[baseIndex * 3 + 2];
        }

        void GetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 + iVert];
            fvNormOut[0] = meshData->normals[baseIndex * 3 + 0];
            fvNormOut[1] = meshData->normals[baseIndex * 3 + 1];
            fvNormOut[2] = meshData->normals[baseIndex * 3 + 2];
        }

        void GetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 * iVert];
            fvTexcOut[0] = meshData->texcoords[baseIndex * 2 + 0];
            fvTexcOut[1] = meshData->texcoords[baseIndex * 2 + 1];
        }

        // either (or both) of the two setTSpace callbacks can be set.
        // The call-back m_setTSpaceBasic() is sufficient for basic normal mapping.

        // This function is used to return the tangent and fSign to the application.
        // fvTangent is a unit length vector.
        // For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
        // bitangent = fSign * cross(vN, tangent);
        // Note that the results are returned unindexed. It is possible to generate a new index list
        // But averaging/overwriting tangent spaces by using an already existing index list WILL produce INCRORRECT results.
        // DO NOT! use an already existing index list.
        void SetTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 + iVert];

            meshData->tangents[baseIndex * 4 + 0] = fvTangent[0];
            meshData->tangents[baseIndex * 4 + 1] = fvTangent[1];
            meshData->tangents[baseIndex * 4 + 2] = fvTangent[2];
            meshData->tangents[baseIndex * 4 + 3] = fSign;
        }
    }

    namespace MikktsInterface1
    {
        struct PKMeshData
        {
            float* vertices = nullptr;
            unsigned int stride = 0;
            unsigned int vertexOffset = 0;
            unsigned int normalOffset = 0;
            unsigned int tangentOffset = 0;
            unsigned int texcoordOffset = 0;
            const unsigned int* indices = nullptr;
            unsigned int vcount = 0;
            unsigned int icount = 0;
        };

        // Returns the number of faces (triangles/quads) on the mesh to be processed.
        int GetNumFaces(const SMikkTSpaceContext* pContext)
        {
            return reinterpret_cast<PKMeshData*>(pContext->m_pUserData)->icount / 3;
        }

        // Returns the number of vertices on face number iFace
        // iFace is a number in the range {0, 1, ..., getNumFaces()-1}
        int GetNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
        {
            return 3;
        }

        // returns the position/normal/texcoord of the referenced face of vertex number iVert.
        // iVert is in the range {0,1,2} for triangles and {0,1,2,3} for quads.
        void GetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 + iVert];
            auto vertex = meshData->vertices + baseIndex * meshData->stride + meshData->vertexOffset;
            fvPosOut[0] = vertex[0];
            fvPosOut[1] = vertex[1];
            fvPosOut[2] = vertex[2];
        }

        void GetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 + iVert];
            auto normal = meshData->vertices + baseIndex * meshData->stride + meshData->normalOffset;
            fvNormOut[0] = normal[0];
            fvNormOut[1] = normal[1];
            fvNormOut[2] = normal[2];
        }

        void GetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 + iVert];
            auto texcoord = meshData->vertices + baseIndex * meshData->stride + meshData->texcoordOffset;
            fvTexcOut[0] = texcoord[0];
            fvTexcOut[1] = texcoord[1];
        }

        // either (or both) of the two setTSpace callbacks can be set.
        // The call-back m_setTSpaceBasic() is sufficient for basic normal mapping.

        // This function is used to return the tangent and fSign to the application.
        // fvTangent is a unit length vector.
        // For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
        // bitangent = fSign * cross(vN, tangent);
        // Note that the results are returned unindexed. It is possible to generate a new index list
        // But averaging/overwriting tangent spaces by using an already existing index list WILL produce INCRORRECT results.
        // DO NOT! use an already existing index list.
        void SetTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<PKMeshData*>(pContext->m_pUserData);
            auto baseIndex = meshData->indices[iFace * 3 + iVert];
            auto tangent = meshData->vertices + baseIndex * meshData->stride + meshData->tangentOffset;
            tangent[0] = fvTangent[0];
            tangent[1] = fvTangent[1];
            tangent[2] = fvTangent[2];
            tangent[3] = fSign;
        }
    }

    void CalculateNormals(const float3* vertices, const uint* indices, float3* normals, uint vcount, uint icount, float sign)
    {
        for (uint i = 0, j = 0; i < icount; i += 3)
        {
            auto i0 = indices[i + 0];
            auto i1 = indices[i + 1];
            auto i2 = indices[i + 2];
            auto v0 = vertices[i0];
            auto v1 = vertices[i1];
            auto v2 = vertices[i2];

            auto tangent = glm::normalize(v1 - v0);
            auto binormal = glm::normalize(v2 - v0);
            auto normal = glm::normalize(glm::cross(tangent, binormal));
            normals[i0] += normal;
            normals[i1] += normal;
            normals[i2] += normal;
        }

        for (uint i = 0; i < vcount; ++i)
        {
            normals[i] = glm::normalize(normals[i]) * sign;
        }

    }

    void CalculateTangents(const float3* vertices, const float3* normals, const float2* texcoords, const uint* indices, float4* tangents, uint vcount, uint icount)
    {
        MikktsInterface0::PKMeshData data;
        data.vertices = reinterpret_cast<const float*>(vertices);
        data.normals = reinterpret_cast<const float*>(normals);
        data.tangents = reinterpret_cast<float*>(tangents);
        data.texcoords = reinterpret_cast<const float*>(texcoords);
        data.indices = indices;
        data.vcount = vcount;
        data.icount = icount;

        SMikkTSpaceInterface mikttInterface;
        mikttInterface.m_getNumFaces = MikktsInterface0::GetNumFaces;
        mikttInterface.m_getNumVerticesOfFace = MikktsInterface0::GetNumVerticesOfFace;
        mikttInterface.m_getPosition = MikktsInterface0::GetPosition;
        mikttInterface.m_getNormal = MikktsInterface0::GetNormal;
        mikttInterface.m_getTexCoord = MikktsInterface0::GetTexCoord;
        mikttInterface.m_setTSpaceBasic = MikktsInterface0::SetTSpaceBasic;
        mikttInterface.m_setTSpace = nullptr;

        SMikkTSpaceContext context;
        context.m_pInterface = &mikttInterface;
        context.m_pUserData = &data;

        PK_THROW_ASSERT(genTangSpaceDefault(&context), "Failed to calculate tangents");
    }

    void CalculateTangents(void* vertices, uint stride, uint vertexOffset, uint normalOffset, uint tangentOffset, uint texcoordOffset, const uint* indices, uint vcount, uint icount)
    {
        MikktsInterface1::PKMeshData data;
        data.vertices = reinterpret_cast<float*>(vertices);
        data.stride = stride;
        data.vertexOffset = vertexOffset;
        data.normalOffset = normalOffset;
        data.tangentOffset = tangentOffset;
        data.texcoordOffset = texcoordOffset;
        data.indices = indices;
        data.vcount = vcount;
        data.icount = icount;

        SMikkTSpaceInterface mikttInterface;
        mikttInterface.m_getNumFaces = MikktsInterface1::GetNumFaces;
        mikttInterface.m_getNumVerticesOfFace = MikktsInterface1::GetNumVerticesOfFace;
        mikttInterface.m_getPosition = MikktsInterface1::GetPosition;
        mikttInterface.m_getNormal = MikktsInterface1::GetNormal;
        mikttInterface.m_getTexCoord = MikktsInterface1::GetTexCoord;
        mikttInterface.m_setTSpaceBasic = MikktsInterface1::SetTSpaceBasic;
        mikttInterface.m_setTSpace = nullptr;

        SMikkTSpaceContext context;
        context.m_pInterface = &mikttInterface;
        context.m_pUserData = &data;

        PK_THROW_ASSERT(genTangSpaceDefault(&context), "Failed to calculate tangents");
    }

    Ref<Mesh> GetBox(const float3& offset, const float3& extents)
    {
        float3 p0 = { offset.x - extents.x, offset.y - extents.y, offset.z + extents.z };
        float3 p1 = { offset.x + extents.x, offset.y - extents.y, offset.z + extents.z };
        float3 p2 = { offset.x + extents.x, offset.y - extents.y, offset.z - extents.z };
        float3 p3 = { offset.x - extents.x, offset.y - extents.y, offset.z - extents.z };
        float3 p4 = { offset.x - extents.x, offset.y + extents.y, offset.z + extents.z };
        float3 p5 = { offset.x + extents.x, offset.y + extents.y, offset.z + extents.z };
        float3 p6 = { offset.x + extents.x, offset.y + extents.y, offset.z - extents.z };
        float3 p7 = { offset.x - extents.x, offset.y + extents.y, offset.z - extents.z };

        float3 up = PK_FLOAT3_UP;
        float3 down = PK_FLOAT3_DOWN;
        float3 front = PK_FLOAT3_FORWARD;
        float3 back = PK_FLOAT3_BACKWARD;
        float3 left = PK_FLOAT3_LEFT;
        float3 right = PK_FLOAT3_RIGHT;

        float2 uv00 = { 0.0f, 0.0f };
        float2 uv10 = { 1.0f, 0.0f };
        float2 uv01 = { 0.0f, 1.0f };
        float2 uv11 = { 1.0f, 1.0f };

        Structs::Vertex_Full vertices[] =
        {
            // Bottom
            { p0, down, float4(front, 1), uv11 },
            { p1, down, float4(front, 1), uv01 },
            { p2, down, float4(front, 1), uv00 },
            { p3, down, float4(front, 1), uv10 },

            // Left
            { p7, left, float4(down, 1), uv11 },
            { p4, left, float4(down, 1), uv01 },
            { p0, left, float4(down, 1), uv00 },
            { p3, left, float4(down, 1), uv10 },

            // Front
            { p4, front, float4(down, 1), uv11 },
            { p5, front, float4(down, 1), uv01 },
            { p1, front, float4(down, 1), uv00 },
            { p0, front, float4(down, 1), uv10 },

            // Back
            { p6, back, float4(up, 1), uv11 },
            { p7, back, float4(up, 1), uv01 },
            { p3, back, float4(up, 1), uv00 },
            { p2, back, float4(up, 1), uv10 },

            // Right
            { p5, right, float4(up, 1), uv11 },
            { p6, right, float4(up, 1), uv01 },
            { p2, right, float4(up, 1), uv00 },
            { p1, right, float4(up, 1), uv10 },

            // Top
            { p7, up, float4(back, 1), uv11 },
            { p6, up, float4(back, 1), uv01 },
            { p5, up, float4(back, 1), uv00 },
            { p4, up, float4(back, 1), uv10 }
        };

        unsigned int indices[] =
        {
            // Bottom
            3, 1, 0, 3, 2, 1,

            // Left
            3 + 4 * 1, 1 + 4 * 1, 0 + 4 * 1,
            3 + 4 * 1, 2 + 4 * 1, 1 + 4 * 1,

            // Front
            3 + 4 * 2, 1 + 4 * 2, 0 + 4 * 2,
            3 + 4 * 2, 2 + 4 * 2, 1 + 4 * 2,

            // Back
            3 + 4 * 3, 1 + 4 * 3, 0 + 4 * 3,
            3 + 4 * 3, 2 + 4 * 3, 1 + 4 * 3,

            // Right
            3 + 4 * 4, 1 + 4 * 4, 0 + 4 * 4,
            3 + 4 * 4, 2 + 4 * 4, 1 + 4 * 4,

            // Top
            3 + 4 * 5, 1 + 4 * 5, 0 + 4 * 5,
            3 + 4 * 5, 2 + 4 * 5, 1 + 4 * 5,
        };

        BufferLayout layout = 
        { 
            { ElementType::Float3, PK_VS_POSITION }, 
            { ElementType::Float3, PK_VS_NORMAL }, 
            { ElementType::Float3, PK_VS_TANGENT }, 
            { ElementType::Float2, PK_VS_TEXCOORD0 } 
        };

        return CreateRef<Mesh>
        (
            Buffer::CreateVertex(layout, vertices, 24),
            Buffer::CreateIndex(ElementType::Uint, indices, 36),
            PK::Math::BoundingBox::CenterExtents(offset, extents)
        );
    }

    Ref<Mesh> GetQuad(const float2& min, const float2& max)
    {
        float vertices[] =
        {
             min.x, min.y, 0.0f,
             0.0,   0.0f,
             min.x, max.y, 0.0f,
             0.0f,  1.0f,
             max.x, max.y, 0.0f,
             1.0f,  1.0f,
             max.x, min.y, 0.0f,
             1.0f,  0.0f
        };

        unsigned int indices[] =
        {
            0,1,2,
            2,3,0
        };

        return CreateRef<Mesh>
        (
            Buffer::CreateVertex({ {ElementType::Float3, PK_VS_POSITION }, { ElementType::Float2, PK_VS_TEXCOORD0 } }, vertices, 4),
            Buffer::CreateIndex(ElementType::Uint, indices, 6)
        );
    }

    Ref<VirtualMesh> GetPlane(Ref<Mesh> baseMesh, const float2& center, const float2& extents, uint2 resolution)
    {
        auto vcount = resolution.x * resolution.y * 4;
        auto icount = resolution.x * resolution.y * 6;
        auto* vertices = PK_CONTIGUOUS_ALLOC(Structs::Vertex_Full, vcount);
        auto* indices = PK_CONTIGUOUS_ALLOC(uint, icount);
        auto isize = float3(extents.x / resolution.x, extents.y / resolution.y, 0.0f) * 2.0f;
        auto min = float3(center - extents, 0);

        for (uint x = 0; x < resolution.x; ++x)
        for (uint y = 0; y < resolution.y; ++y)
        {
            auto vmin = min + isize * float3(x, y, 0);
            auto baseVertex = (y * resolution.x + x) * 4;
            auto baseIndex = (y * resolution.x + x) * 6;

            vertices[baseVertex + 0] = { vmin + isize.zzz, PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(0, 0) };
            vertices[baseVertex + 1] = { vmin + isize.zyz, PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(0, 1) };
            vertices[baseVertex + 2] = { vmin + isize.xyz, PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(1, 1) };
            vertices[baseVertex + 3] = { vmin + isize.xzz, PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(1, 0) };

            indices[baseIndex + 0] = baseVertex + 0;
            indices[baseIndex + 1] = baseVertex + 1;
            indices[baseIndex + 2] = baseVertex + 2;

            indices[baseIndex + 3] = baseVertex + 2;
            indices[baseIndex + 4] = baseVertex + 3;
            indices[baseIndex + 5] = baseVertex + 0;
        }

        SubMesh submesh = { 0u, vcount, 0u, icount, BoundingBox::CenterExtents({ center.x, center.y, 0.0f }, { extents.x, extents.y, 0.0f }) };
        SubmeshRangeAllocationInfo allocInfo{};
        allocInfo.pVertices = vertices;
        allocInfo.pIndices = indices;
        allocInfo.vertexLayout =
        {
            { ElementType::Float3, PK_VS_POSITION },
            { ElementType::Float3, PK_VS_NORMAL },
            { ElementType::Float4, PK_VS_TANGENT },
            { ElementType::Float2, PK_VS_TEXCOORD0 }
        };

        allocInfo.pSubmeshes = &submesh;
        allocInfo.indexType = ElementType::Uint;
        allocInfo.vertexCount = vcount;
        allocInfo.indexCount = icount;
        allocInfo.submeshCount = 1u;

        CalculateTangents(reinterpret_cast<float*>(vertices), allocInfo.vertexLayout.GetStride() / 4, 0, 3, 6, 10, indices, vcount, icount);

        auto virtualMesh = CreateRef<VirtualMesh>(allocInfo, baseMesh);

        free(vertices);
        free(indices);

        return virtualMesh;
    }

    Ref<VirtualMesh> GetSphere(Ref<Mesh> baseMesh, const float3& offset, const float radius)
    {
        const int longc = 24;
        const int lattc = 16;
        const int vcount = (longc + 1) * lattc + 2;

        //Vertex_Full
        auto vertices = PK_CONTIGUOUS_ALLOC(Structs::Vertex_Full, vcount);

        vertices[0].position = PK_FLOAT3_UP * radius;

        for (int lat = 0; lat < lattc; lat++)
        {
            float a1 = PK_FLOAT_PI * (float)(lat + 1) / (lattc + 1);
            float sin1 = sin(a1);
            float cos1 = cos(a1);

            for (int lon = 0; lon <= longc; lon++)
            {
                float a2 = PK_FLOAT_2PI * (float)(lon == longc ? 0 : lon) / longc;
                float sin2 = sin(a2);
                float cos2 = cos(a2);
                vertices[lon + lat * (longc + 1) + 1].position = float3(sin1 * cos2, cos1, sin1 * sin2) * radius;
            }
        }

        vertices[vcount - 1].position = PK_FLOAT3_UP * -radius;

        for (int n = 0; n < vcount; ++n)
        {
            vertices[n].normal = glm::normalize(vertices[n].position);
        }

        vertices[0].texcoord = PK_FLOAT2_UP;
        vertices[vcount - 1].texcoord = PK_FLOAT2_ZERO;

        for (int lat = 0; lat < lattc; lat++)
        {
            for (int lon = 0; lon <= longc; lon++)
            {
                vertices[lon + lat * (longc + 1) + 1].texcoord = float2((float)lon / longc, 1.0f - (float)(lat + 1) / (lattc + 1));
            }
        }

        const int facec = vcount;
        const int triscount = facec * 2;
        const int icount = triscount * 3;
        auto indices = PK_CONTIGUOUS_ALLOC(uint, icount);

        //Top Cap
        int i = 0;

        for (int lon = 0; lon < longc; lon++)
        {
            indices[i++] = lon + 2;
            indices[i++] = lon + 1;
            indices[i++] = 0;
        }

        //Middle
        for (int lat = 0; lat < lattc - 1; lat++)
        {
            for (int lon = 0; lon < longc; lon++)
            {
                int current = lon + lat * (longc + 1) + 1;
                int next = current + longc + 1;

                indices[i++] = current;
                indices[i++] = current + 1;
                indices[i++] = next + 1;

                indices[i++] = current;
                indices[i++] = next + 1;
                indices[i++] = next;
            }
        }

        //Bottom Cap
        for (int lon = 0; lon < longc; lon++)
        {
            indices[i++] = vcount - 1;
            indices[i++] = vcount - (lon + 2) - 1;
            indices[i++] = vcount - (lon + 1) - 1;
        }

        SubMesh submesh = { 0u, vcount, 0u, icount, BoundingBox::CenterExtents(offset, PK_FLOAT3_ONE * radius) };
        SubmeshRangeAllocationInfo allocInfo{};
        allocInfo.pVertices = vertices;
        allocInfo.pIndices = indices;
        allocInfo.vertexLayout =
        {
            { ElementType::Float3, PK_VS_POSITION },
            { ElementType::Float3, PK_VS_NORMAL },
            { ElementType::Float4, PK_VS_TANGENT },
            { ElementType::Float2, PK_VS_TEXCOORD0 }
        };

        allocInfo.pSubmeshes = &submesh;
        allocInfo.indexType = ElementType::Uint;
        allocInfo.vertexCount = vcount;
        allocInfo.indexCount = icount;
        allocInfo.submeshCount = 1u;

        CalculateTangents(reinterpret_cast<float*>(vertices), allocInfo.vertexLayout.GetStride() / 4, 0, 3, 6, 10, indices, vcount, icount);

        auto virtualMesh = CreateRef<VirtualMesh>(allocInfo, baseMesh);

        free(vertices);
        free(indices);

        return virtualMesh;
    }
}