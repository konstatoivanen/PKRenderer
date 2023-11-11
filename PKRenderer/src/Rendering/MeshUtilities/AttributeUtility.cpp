#include "PrecompiledHeader.h"
#include <mikktspace/mikktspace.h>
#include "Core/Services/Log.h"
#include "AttributeUtility.h"

namespace PK::Rendering::MeshUtilities
{
    using namespace PK::Math;

    namespace MikktsInterface0
    {
        struct PKMeshData
        {
            const float* vertices = nullptr;
            const float* normals = nullptr;
            float* tangents = nullptr;
            const float* texcoords = nullptr;
            const uint32_t* indices = nullptr;
            uint32_t vcount = 0;
            uint32_t icount = 0;
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
            uint32_t stride = 0;
            uint32_t vertexOffset = 0;
            uint32_t normalOffset = 0;
            uint32_t tangentOffset = 0;
            uint32_t texcoordOffset = 0;
            const uint32_t* indices = nullptr;
            uint32_t vcount = 0;
            uint32_t icount = 0;
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

    void CalculateNormals(const float3* vertices, const uint32_t* indices, float3* normals, uint32_t vcount, uint32_t icount, float sign)
    {
        for (auto i = 0u, j = 0u; i < icount; i += 3u)
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

        for (auto i = 0u; i < vcount; ++i)
        {
            normals[i] = glm::normalize(normals[i]) * sign;
        }
    }

    void CalculateTangents(const float3* vertices, const float3* normals, const float2* texcoords, const uint32_t* indices, float4* tangents, uint32_t vcount, uint32_t icount)
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

    void CalculateTangents(void* vertices, uint32_t stride, uint32_t vertexOffset, uint32_t normalOffset, uint32_t tangentOffset, uint32_t texcoordOffset, const uint32_t* indices, uint32_t vcount, uint32_t icount)
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
}