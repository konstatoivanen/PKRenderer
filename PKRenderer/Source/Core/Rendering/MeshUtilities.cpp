#include "PrecompiledHeader.h"
#include <mikktspace/mikktspace.h>
#include "Core/CLI/Log.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Rendering/MeshStaticCollection.h"
#include "Core/Rendering/MeshStaticAsset.h"
#include "Core/Rendering/Mesh.h"
#include "MeshUtilities.h"

namespace PK::MeshUtilities
{
    namespace MikktsInterface
    {
        static int GetNumFaces(const SMikkTSpaceContext* pContext) { return reinterpret_cast<GeometryContext*>(pContext->m_pUserData)->countIndex / 3; }
        
        static int GetNumVerticesOfFace([[maybe_unused]] const SMikkTSpaceContext* pContext, [[maybe_unused]] const int iFace) { return 3; }
        
        static void GetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<GeometryContext*>(pContext->m_pUserData);
            auto pPosition =  meshData->pPositions + meshData->pIndices[iFace * 3 + iVert] * meshData->stridePositionsf32;
            fvPosOut[0] = pPosition[0];
            fvPosOut[1] = pPosition[1];
            fvPosOut[2] = pPosition[2];
        }

        static void GetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<GeometryContext*>(pContext->m_pUserData);
            auto pNormal = meshData->pNormals + meshData->pIndices[iFace * 3 + iVert] * meshData->strideNormalsf32;
            fvNormOut[0] = pNormal[0];
            fvNormOut[1] = pNormal[1];
            fvNormOut[2] = pNormal[2];
        }

        static void GetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<GeometryContext*>(pContext->m_pUserData);
            auto pTexcoord = meshData->pTexcoords + meshData->pIndices[iFace * 3 + iVert] * meshData->strideTexcoordsf32;
            fvTexcOut[0] = pTexcoord[0];
            fvTexcOut[1] = pTexcoord[1];
        }

        static void SetTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<GeometryContext*>(pContext->m_pUserData);
            auto pTangent = meshData->pTangents + meshData->pIndices[iFace * 3 + iVert] * meshData->strideTangentsf32;
            pTangent[0] = fvTangent[0];
            pTangent[1] = fvTangent[1];
            pTangent[2] = fvTangent[2];
            pTangent[3] = fSign;
        }
    }

    void AlignVertexStreams(char* vertices, size_t count, const VertexStreamLayout& src, const VertexStreamLayout& dst)
    {
        auto needsAlignment = false;
        uint32_t* remap = PK_STACK_ALLOC(uint32_t, dst.GetCount());

        for (auto i = 0u; i < dst.GetCount(); ++i)
        {
            const auto& vdst = dst[i];

            for (auto j = 0u; j < src.GetCount(); ++j)
            {
                const auto& vsrc = src[j];

                if (vsrc.name == vdst.name)
                {
                    PK_THROW_ASSERT(vsrc.size == vdst.size, "Element '%s' size missmatch '%u' & '%u'", vdst.name.c_str(), vdst.size, vsrc.size);
                    PK_THROW_ASSERT(vsrc.inputRate == vdst.inputRate, "Element '%s' input rate missmatch '%i' & '%i'", vdst.name.c_str(), (int)vdst.inputRate, (int)vsrc.inputRate);
                    needsAlignment |= vsrc.stream != vdst.stream || vsrc.offset != vdst.offset;
                    remap[i] = j;
                    break;
                }
            }
        }


        PK_THROW_ASSERT(dst.GetStride() == src.GetStride(), "Layout stride missmatch!");

        if (needsAlignment)
        {
            auto buffer = (char*)malloc(count * dst.GetStride());

            for (auto i = 0u; i < dst.GetCount(); ++i)
            {
                const auto& vdst = dst[i];
                const auto& vsrc = src[remap[i]];
                const auto srcOffset = vsrc.stream == 0 ? 0ull : src.GetStride(vsrc.stream - 1) * count;
                const auto dstOffset = vdst.stream == 0 ? 0ull : dst.GetStride(vdst.stream - 1) * count;

                for (auto i = 0u; i < count; ++i)
                {
                    memcpy(buffer + dstOffset + vdst.stride * i + vdst.offset, vertices + srcOffset + vsrc.stride * i + vsrc.offset, vdst.size);
                }
            }

            memcpy(vertices, buffer, count * dst.GetStride());
            free(buffer);
        }
    }

    void CalculateNormals(GeometryContext* ctx, float sign)
    {
        for (auto i = 0u; i < ctx->countIndex; i += 3u)
        {
            const auto& v0 = *reinterpret_cast<float3*>(ctx->pPositions + ctx->pIndices[i + 0] * ctx->stridePositionsf32);
            const auto& v1 = *reinterpret_cast<float3*>(ctx->pPositions + ctx->pIndices[i + 1] * ctx->stridePositionsf32);
            const auto& v2 = *reinterpret_cast<float3*>(ctx->pPositions + ctx->pIndices[i + 2] * ctx->stridePositionsf32);
            auto& n0 = *reinterpret_cast<float3*>(ctx->pNormals + ctx->pIndices[i + 0] * ctx->strideNormalsf32);
            auto& n1 = *reinterpret_cast<float3*>(ctx->pNormals + ctx->pIndices[i + 1] * ctx->strideNormalsf32);
            auto& n2 = *reinterpret_cast<float3*>(ctx->pNormals + ctx->pIndices[i + 2] * ctx->strideNormalsf32);

            auto tangent = glm::normalize(v1 - v0);
            auto binormal = glm::normalize(v2 - v0);
            auto normal = glm::normalize(glm::cross(tangent, binormal));
            n0 += normal;
            n1 += normal;
            n2 += normal;
        }

        for (auto i = 0u; i < ctx->countVertex; ++i)
        {
            auto& normal = *reinterpret_cast<float3*>(ctx->pNormals + i * ctx->strideNormalsf32);
            normal = glm::normalize(normal) * sign;
        }
    }

    void CalculateTangents(GeometryContext* ctx)
    {
        SMikkTSpaceInterface mikttInterface;
        mikttInterface.m_getNumFaces = MikktsInterface::GetNumFaces;
        mikttInterface.m_getNumVerticesOfFace = MikktsInterface::GetNumVerticesOfFace;
        mikttInterface.m_getPosition = MikktsInterface::GetPosition;
        mikttInterface.m_getNormal = MikktsInterface::GetNormal;
        mikttInterface.m_getTexCoord = MikktsInterface::GetTexCoord;
        mikttInterface.m_setTSpaceBasic = MikktsInterface::SetTSpaceBasic;
        mikttInterface.m_setTSpace = nullptr;

        SMikkTSpaceContext context;
        context.m_pInterface = &mikttInterface;
        context.m_pUserData = ctx;

        PK_THROW_ASSERT(genTangSpaceDefault(&context), "Failed to calculate tangents");
    }


    MeshletBuildData BuildMeshletsMonotone(GeometryContext* ctx)
    {
        struct MeshletIndexInfo
        {
            uint32_t vertex_offset;
            uint32_t triangle_offset;
            uint32_t vertex_count;
            uint32_t triangle_count;
        };

        MeshletBuildData output{};

        assert(ctx->countIndex % 3 == 0);

        const auto max_vertices = PKAssets::PK_MESHLET_MAX_VERTICES;
        const auto max_triangles = PKAssets::PK_MESHLET_MAX_TRIANGLES;
        const auto max_vertices_conservative = PKAssets::PK_MESHLET_MAX_VERTICES - 2;

        auto meshlet_limit_vertices = (ctx->countIndex + max_vertices_conservative - 1) / max_vertices_conservative;
        auto meshlet_limit_triangles = (ctx->countIndex / 3 + PKAssets::PK_MESHLET_MAX_TRIANGLES - 1) / PKAssets::PK_MESHLET_MAX_TRIANGLES;
        auto max_meshlets = meshlet_limit_vertices > meshlet_limit_triangles ? meshlet_limit_vertices : meshlet_limit_triangles;

        output.meshlets.resize(max_meshlets);
        output.vertices.resize(max_meshlets * PKAssets::PK_MESHLET_MAX_VERTICES);
        output.indices.resize(max_meshlets * PKAssets::PK_MESHLET_MAX_TRIANGLES * 3);

        auto* meshlet_vertices = reinterpret_cast<uint32_t*>(calloc(sizeof(uint32_t), max_meshlets * PKAssets::PK_MESHLET_MAX_VERTICES));
        auto* meshlet_indices = output.indices.data();
        auto* used = reinterpret_cast<uint8_t*>(malloc(ctx->countVertex));
        memset(used, -1, ctx->countVertex);

        memcpy(output.submesh.bbmin, glm::value_ptr(ctx->aabb.min), sizeof(float3));
        memcpy(output.submesh.bbmax, glm::value_ptr(ctx->aabb.max), sizeof(float3));

        auto pack_meshlet_vertex = [](GeometryContext* ctx, PKAssets::PKMeshletVertex* out_vertex, uint32_t index)
        {
            *out_vertex = PKAssets::PackPKMeshletVertex(
                ctx->pPositions + index * ctx->stridePositionsf32,
                ctx->pTexcoords + index * ctx->strideTexcoordsf32,
                ctx->pNormals + index * ctx->strideNormalsf32,
                ctx->pTangents + index * ctx->strideTangentsf32,
                nullptr,
                glm::value_ptr(ctx->aabb.min),
                glm::value_ptr(ctx->aabb.max));
        };

        auto pack_meshlet = [](GeometryContext* ctx, PKAssets::PKMeshlet* out_meshlet, MeshletIndexInfo meshlet, const uint32_t* meshlet_vertices, const uint8_t* meshlet_indices)
        {
            float3 center = PK_FLOAT3_ZERO, extents = PK_FLOAT3_ZERO, cone_apex = PK_FLOAT3_ZERO;
            sbyte3 cone_axis_s8{};
            sbyte cone_cutoff_s8 = 0;

            // Compute bounds
            {
                const auto index_count = meshlet.triangle_count * 3;
                const auto vertex_stride_float = ctx->stridePositionsf32;
                float3 normals[PKAssets::PK_MESHLET_MAX_TRIANGLES * 3];
                float3 corners[PKAssets::PK_MESHLET_MAX_TRIANGLES * 3][3];
                size_t valid_tri_count = 0;

                for (size_t i = 0; i < index_count; i += 3)
                {
                    bool isValid = false;
                    const auto a = ctx->pPositions + vertex_stride_float * meshlet_vertices[meshlet.vertex_offset + meshlet_indices[meshlet.triangle_offset + i + 0]];
                    const auto b = ctx->pPositions + vertex_stride_float * meshlet_vertices[meshlet.vertex_offset + meshlet_indices[meshlet.triangle_offset + i + 1]];
                    const auto c = ctx->pPositions + vertex_stride_float * meshlet_vertices[meshlet.vertex_offset + meshlet_indices[meshlet.triangle_offset + i + 2]];
                    normals[valid_tri_count] = Math::GetTriangleNormal(a, b, c, isValid);
                    corners[valid_tri_count][0] = glm::make_vec3(a);
                    corners[valid_tri_count][1] = glm::make_vec3(b);
                    corners[valid_tri_count][2] = glm::make_vec3(c);
                    valid_tri_count += (size_t)isValid;
                }

                // degenerate cluster, no valid triangles => trivial reject (cone data is 0)
                if (valid_tri_count > 0)
                {
                    const auto psphere = Math::ComputeBoundingSphere(corners[0], valid_tri_count * 3);
                    const auto nsphere = Math::ComputeBoundingSphere(normals, valid_tri_count);
                    const auto axis = Math::SafeNormalize(nsphere.xyz);
                    auto aabb = Math::ComputeBoundingBox(corners[0], valid_tri_count * 3);
                    center = aabb.GetCenter();
                    extents = aabb.GetExtents();
                    cone_cutoff_s8 = 127;

                    auto minCosA = 1.0f;

                    for (auto i = 0u; i < valid_tri_count; ++i)
                    {
                        minCosA = glm::min(minCosA, glm::dot(normals[i], axis));
                    }

                    if (minCosA > 0.1f)
                    {
                        auto maxt = 0.0f;

                        for (auto i = 0u; i < valid_tri_count; ++i)
                        {
                            maxt = glm::max(maxt, glm::dot(psphere.xyz - corners[i][0], normals[i]) / glm::dot(axis, normals[i]));
                        }

                        cone_apex = (psphere.xyz - axis) * maxt;
                        cone_axis_s8 = Math::QuantizeSNorm(axis, 8);
                        const auto cone_axis_s8_e = glm::abs(float3(cone_axis_s8) / 127.0f - axis);
                        const auto cone_cutoff = int(127 * (sqrtf(1.0f - minCosA * minCosA) + cone_axis_s8_e.x + cone_axis_s8_e.y + cone_axis_s8_e.z) + 1);
                        cone_cutoff_s8 = (cone_cutoff > 127) ? 127 : (signed char)(cone_cutoff);
                    }
                }
            }

            *out_meshlet = PKAssets::PackPKMeshlet(
                meshlet.vertex_offset,
                meshlet.triangle_offset / 3u,
                meshlet.vertex_count,
                meshlet.triangle_count,
                glm::value_ptr(cone_axis_s8),
                cone_cutoff_s8,
                glm::value_ptr(cone_apex),
                glm::value_ptr(center),
                glm::value_ptr(extents),
                glm::value_ptr(center),
                -1.0f,
                glm::value_ptr(center),
                PKAssets::PK_MESHLET_LOD_MAX_ERROR);
        };

        MeshletIndexInfo meshlet = {};

        for (size_t i = 0; i < ctx->countIndex; i += 3)
        {
            uint32_t a = ctx->pIndices[i + 0], b = ctx->pIndices[i + 1], c = ctx->pIndices[i + 2];
            assert(a < ctx->countVertex && b < ctx->countVertex && c < ctx->countVertex);

            // appends triangle to the meshlet and writes previous meshlet to the output if full
            uint32_t used_extra = (used[a] == 0xff) + (used[b] == 0xff) + (used[c] == 0xff);

            if (meshlet.vertex_count + used_extra > max_vertices || meshlet.triangle_count >= max_triangles)
            {
                pack_meshlet(ctx, &output.meshlets[output.meshlet_count++], meshlet, meshlet_vertices, meshlet_indices);
                meshlet.vertex_offset += meshlet.vertex_count;
                meshlet.triangle_offset += meshlet.triangle_count * 3;
                meshlet.vertex_count = 0;
                meshlet.triangle_count = 0;
                memset(used, -1, ctx->countVertex);
            }

            if (used[a] == 0xff)
            {
                used[a] = (uint8_t)meshlet.vertex_count;
                pack_meshlet_vertex(ctx, &output.vertices[meshlet.vertex_offset + meshlet.vertex_count], a);
                meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = a;
            }

            if (used[b] == 0xff)
            {
                used[b] = (uint8_t)meshlet.vertex_count;
                pack_meshlet_vertex(ctx, &output.vertices[meshlet.vertex_offset + meshlet.vertex_count], b);
                meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = b;
            }

            if (used[c] == 0xff)
            {
                used[c] = (uint8_t)meshlet.vertex_count;
                pack_meshlet_vertex(ctx, &output.vertices[meshlet.vertex_offset + meshlet.vertex_count], c);
                meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = c;
            }

            meshlet_indices[meshlet.triangle_offset + meshlet.triangle_count * 3 + 0] = used[a];
            meshlet_indices[meshlet.triangle_offset + meshlet.triangle_count * 3 + 1] = used[b];
            meshlet_indices[meshlet.triangle_offset + meshlet.triangle_count * 3 + 2] = used[c];
            meshlet.triangle_count++;
        }

        if (meshlet.triangle_count)
        {
            pack_meshlet(ctx, &output.meshlets[output.meshlet_count++], meshlet, meshlet_vertices, meshlet_indices);
        }

        // Align triangles array size to 4bytes
        output.index_count = 12u * ((meshlet.triangle_offset + meshlet.triangle_count * 3 + 11ull) / 12u);
        output.vertex_count = meshlet.vertex_offset + meshlet.vertex_count;
        output.submesh.firstMeshlet = 0u;
        output.submesh.meshletCount = output.meshlet_count;

        free(meshlet_vertices);
        free(used);

        return output;
    }

    MeshStatic* CreateMeshStatic(MeshStaticCollection* baseMesh, GeometryContext* ctx, const char* name)
    {
        CalculateTangents(ctx);

        auto meshlets = BuildMeshletsMonotone(ctx);

        MeshStaticDescriptor::SubMesh submesh = { 0u, ctx->countVertex, 0u, ctx->countIndex, ctx->aabb };

        MeshStaticDescriptor desc{};
        desc.name = name;
        desc.regular.pVertices = ctx->pVertices;
        desc.regular.pIndices = ctx->pIndices;
        desc.regular.pSubmeshes = &submesh;
        desc.regular.indexType = ElementType::Uint;
        desc.regular.vertexCount = ctx->countVertex;
        desc.regular.indexCount = ctx->countIndex;
        desc.regular.submeshCount = 1u;
        desc.regular.streamLayout =
        {
            { ElementType::Float3, PK_RHI_VS_NORMAL, 0 },
            { ElementType::Float4, PK_RHI_VS_TANGENT, 0 },
            { ElementType::Float2, PK_RHI_VS_TEXCOORD0, 0 },
            { ElementType::Float3, PK_RHI_VS_POSITION, 1 },
        };
        desc.meshlet.pSubmeshes = &meshlets.submesh;
        desc.meshlet.submeshCount = 1u;
        desc.meshlet.pMeshlets = meshlets.meshlets.data();
        desc.meshlet.meshletCount = meshlets.meshlet_count;
        desc.meshlet.pVertices = meshlets.vertices.data();
        desc.meshlet.vertexCount = meshlets.vertex_count;
        desc.meshlet.pIndices = meshlets.indices.data();
        desc.meshlet.triangleCount = meshlets.index_count / 3u;

        return baseMesh->Allocate(&desc);
    }

    MeshStatic* CreateBoxMeshStatic(MeshStaticCollection* baseMesh, const float3& offset, const float3& extents)
    {
        constexpr auto vcount = 24u;
        constexpr auto icount = 36u;

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

        struct VertexData
        {
            Vertex_NormalTangentTexCoord attributes[vcount];
            float3 positions[vcount];
        } 
        vertexData;

        vertexData.attributes[0] = { down, float4(front, 1), uv11 };
        vertexData.attributes[1] = { down, float4(front, 1), uv01 };
        vertexData.attributes[2] = { down, float4(front, 1), uv00 };
        vertexData.attributes[3] = { down, float4(front, 1), uv10 };
        vertexData.attributes[4] = { left, float4(down, 1), uv11 };
        vertexData.attributes[5] = { left, float4(down, 1), uv01 };
        vertexData.attributes[6] = { left, float4(down, 1), uv00 };
        vertexData.attributes[7] = { left, float4(down, 1), uv10 };
        vertexData.attributes[8] = { front, float4(down, 1), uv11 };
        vertexData.attributes[9] = { front, float4(down, 1), uv01 };
        vertexData.attributes[10] = { front, float4(down, 1), uv00 };
        vertexData.attributes[11] = { front, float4(down, 1), uv10 };
        vertexData.attributes[12] = { back, float4(up, 1), uv11 };
        vertexData.attributes[13] = { back, float4(up, 1), uv01 };
        vertexData.attributes[14] = { back, float4(up, 1), uv00 };
        vertexData.attributes[15] = { back, float4(up, 1), uv10 };
        vertexData.attributes[16] = { right, float4(up, 1), uv11 };
        vertexData.attributes[17] = { right, float4(up, 1), uv01 };
        vertexData.attributes[18] = { right, float4(up, 1), uv00 };
        vertexData.attributes[19] = { right, float4(up, 1), uv10 };
        vertexData.attributes[20] = { up, float4(back, 1), uv11 };
        vertexData.attributes[21] = { up, float4(back, 1), uv01 };
        vertexData.attributes[22] = { up, float4(back, 1), uv00 };
        vertexData.attributes[23] = { up, float4(back, 1), uv10 };

        vertexData.positions[0] = p0;
        vertexData.positions[1] = p1;
        vertexData.positions[2] = p2;
        vertexData.positions[3] = p3;
        vertexData.positions[4] = p7;
        vertexData.positions[5] = p4;
        vertexData.positions[6] = p0;
        vertexData.positions[7] = p3;
        vertexData.positions[8] = p4;
        vertexData.positions[9] = p5;
        vertexData.positions[10] = p1;
        vertexData.positions[11] = p0;
        vertexData.positions[12] = p6;
        vertexData.positions[13] = p7;
        vertexData.positions[14] = p3;
        vertexData.positions[15] = p2;
        vertexData.positions[16] = p5;
        vertexData.positions[17] = p6;
        vertexData.positions[18] = p2;
        vertexData.positions[19] = p1;
        vertexData.positions[20] = p7;
        vertexData.positions[21] = p6;
        vertexData.positions[22] = p5;
        vertexData.positions[23] = p4;

        uint32_t indices[] =
        {
            3,  1,  0,  3,  2,  1, 
            7,  5,  4,  7,  6,  5, 
            11, 9,  8,  11, 10, 9,
            15, 13, 12, 15, 14, 13,
            19, 17, 16, 19, 18, 17,
            23, 21, 20, 23, 22, 21
        };

        GeometryContext geometryContext;
        geometryContext.pVertices = vertexData.attributes;
        geometryContext.pPositions = reinterpret_cast<float*>(vertexData.positions);
        geometryContext.stridePositionsf32 = sizeof(float3) / sizeof(float);
        geometryContext.pNormals = reinterpret_cast<float*>(vertexData.attributes) + 0u;
        geometryContext.strideNormalsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTangents = reinterpret_cast<float*>(vertexData.attributes) + 3u;
        geometryContext.strideTangentsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTexcoords = reinterpret_cast<float*>(vertexData.attributes) + 7u;
        geometryContext.strideTexcoordsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = BoundingBox::CenterExtents(offset, extents);
        return CreateMeshStatic(baseMesh, &geometryContext, "Primitive_Box");
    }

    MeshStatic* CreateQuadMeshStatic(MeshStaticCollection* baseMesh, const float2& min, const float2& max)
    {
        constexpr auto vcount = 4u;
        constexpr auto icount = 2u;

        struct VertexData
        {
            Vertex_NormalTangentTexCoord attributes[vcount];
            float3 positions[vcount];
        } vertexData;

        vertexData.positions[0] = { min.x, min.y, 0.0f };
        vertexData.positions[1] = { min.x, max.y, 0.0f };
        vertexData.positions[2] = { max.x, max.y, 0.0f };
        vertexData.positions[3] = { max.x, min.y, 0.0f };
        vertexData.attributes[0] = { PK_FLOAT3_UP, float4(PK_FLOAT3_BACKWARD, 1), { 0.0f, 0.0f }};
        vertexData.attributes[1] = { PK_FLOAT3_UP, float4(PK_FLOAT3_BACKWARD, 1), { 0.0f, 1.0f }};
        vertexData.attributes[2] = { PK_FLOAT3_UP, float4(PK_FLOAT3_BACKWARD, 1), { 1.0f, 1.0f }};
        vertexData.attributes[3] = { PK_FLOAT3_UP, float4(PK_FLOAT3_BACKWARD, 1), { 1.0f, 0.0f }};
        uint32_t indices[] = { 0u,1u,2u, 2u,3u,0u };

        GeometryContext geometryContext;
        geometryContext.pVertices = vertexData.attributes;
        geometryContext.pPositions = reinterpret_cast<float*>(vertexData.positions);
        geometryContext.stridePositionsf32 = sizeof(float3) / sizeof(float);
        geometryContext.pNormals = reinterpret_cast<float*>(vertexData.attributes) + 0u;
        geometryContext.strideNormalsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTangents = reinterpret_cast<float*>(vertexData.attributes) + 3u;
        geometryContext.strideTangentsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTexcoords = reinterpret_cast<float*>(vertexData.attributes) + 7u;
        geometryContext.strideTexcoordsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = BoundingBox({ min.x, min.y, 0.0f }, { max.x, max.y, 0.0f });
        return CreateMeshStatic(baseMesh, &geometryContext, "Primitive_Box");
    }

    MeshStatic* CreatePlaneMeshStatic(MeshStaticCollection * baseMesh, const float2 & center, const float2 & extents, uint2 resolution)
    {
        auto vcount = resolution.x * resolution.y * 4;
        auto icount = resolution.x * resolution.y * 6;
        auto bufferSize = sizeof(Vertex_Full) * vcount + sizeof(uint) * icount;
        auto* buffer = malloc(bufferSize);
        auto* attributes = reinterpret_cast<Vertex_NormalTangentTexCoord*>(buffer);
        auto* positions = reinterpret_cast<float3*>(attributes + vcount);
        auto* indices = reinterpret_cast<uint*>(positions + vcount);

        auto isize = float3(extents.x / resolution.x, extents.y / resolution.y, 0.0f) * 2.0f;
        auto min = float3(center - extents, 0);

        for (auto x = 0u; x < resolution.x; ++x)
        for (auto y = 0u; y < resolution.y; ++y)
        {
            auto vmin = min + isize * float3(x, y, 0);
            auto baseVertex = (y * resolution.x + x) * 4;
            auto baseIndex = (y * resolution.x + x) * 6;

            attributes[baseVertex + 0] = { PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(0, 0) };
            attributes[baseVertex + 1] = { PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(0, 1) };
            attributes[baseVertex + 2] = { PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(1, 1) };
            attributes[baseVertex + 3] = { PK_FLOAT3_BACKWARD, PK_FLOAT4_ZERO, float2(1, 0) };

            positions[baseVertex + 0] = { vmin + isize.zzz };
            positions[baseVertex + 1] = { vmin + isize.zyz };
            positions[baseVertex + 2] = { vmin + isize.xyz };
            positions[baseVertex + 3] = { vmin + isize.xzz };

            indices[baseIndex + 0] = baseVertex + 0;
            indices[baseIndex + 1] = baseVertex + 1;
            indices[baseIndex + 2] = baseVertex + 2;

            indices[baseIndex + 3] = baseVertex + 2;
            indices[baseIndex + 4] = baseVertex + 3;
            indices[baseIndex + 5] = baseVertex + 0;
        }

        GeometryContext geometryContext;
        geometryContext.pVertices = buffer;
        geometryContext.pPositions = reinterpret_cast<float*>(positions);
        geometryContext.stridePositionsf32 = sizeof(float3) / sizeof(float);
        geometryContext.pNormals = reinterpret_cast<float*>(attributes) + 0u;
        geometryContext.strideNormalsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTangents = reinterpret_cast<float*>(attributes) + 3u;
        geometryContext.strideTangentsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTexcoords = reinterpret_cast<float*>(attributes) + 7u;
        geometryContext.strideTexcoordsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = BoundingBox::CenterExtents({ center.x, center.y, 0.0f }, { extents.x, extents.y, 0.0f });
        auto virtualMesh = CreateMeshStatic(baseMesh, &geometryContext, "Primitive_Plane"); 

        free(buffer);

        return virtualMesh;
    }

    MeshStatic* CreateSphereMeshStatic(MeshStaticCollection* baseMesh, const float3& offset, const float radius)
    {
        const auto longc = 24;
        const auto lattc = 16;
        const auto vcount = (longc + 1) * lattc + 2;
        const auto icount = ((lattc - 1) * longc * 2 + longc * 2) * 3;

        //Vertex_Full
        auto* buffer = malloc(sizeof(Vertex_Full) * vcount + sizeof(uint) * icount);
        auto* attributes = reinterpret_cast<Vertex_NormalTangentTexCoord*>(buffer);
        auto* positions = reinterpret_cast<float3*>(attributes + vcount);
        auto* indices = reinterpret_cast<uint*>(positions + vcount);

        positions[0] = PK_FLOAT3_UP * radius;

        for (auto lat = 0u; lat < lattc; lat++)
        {
            float a1 = PK_FLOAT_PI * (float)(lat + 1) / (lattc + 1);
            float sin1 = sin(a1);
            float cos1 = cos(a1);

            for (auto lon = 0u; lon <= longc; lon++)
            {
                float a2 = PK_FLOAT_TWO_PI * (float)(lon == longc ? 0 : lon) / longc;
                float sin2 = sin(a2);
                float cos2 = cos(a2);
                positions[lon + lat * (longc + 1) + 1] = float3(sin1 * cos2, cos1, sin1 * sin2) * radius;
            }
        }

        positions[vcount - 1] = PK_FLOAT3_UP * -radius;

        for (int n = 0; n < vcount; ++n)
        {
            attributes[n].in_NORMAL = glm::normalize(positions[n]);
        }

        attributes[0].in_TEXCOORD0 = PK_FLOAT2_UP;
        attributes[vcount - 1].in_TEXCOORD0 = PK_FLOAT2_ZERO;

        for (auto lat = 0u; lat < lattc; lat++)
        {
            for (int lon = 0u; lon <= longc; lon++)
            {
                attributes[lon + lat * (longc + 1) + 1].in_TEXCOORD0 = float2((float)lon / longc, 1.0f - (float)(lat + 1) / (lattc + 1));
            }
        }


        //Top Cap
        auto i = 0u;

        for (auto lon = 0u; lon < longc; lon++)
        {
            indices[i++] = lon + 2;
            indices[i++] = lon + 1;
            indices[i++] = 0;
        }

        //Middle
        for (auto lat = 0u; lat < lattc - 1; lat++)
        {
            for (auto lon = 0u; lon < longc; lon++)
            {
                auto current = lon + lat * (longc + 1) + 1;
                auto next = current + longc + 1;

                indices[i++] = current;
                indices[i++] = current + 1;
                indices[i++] = next + 1;

                indices[i++] = current;
                indices[i++] = next + 1;
                indices[i++] = next;
            }
        }

        //Bottom Cap
        for (auto lon = 0u; lon < longc; lon++)
        {
            indices[i++] = vcount - 1;
            indices[i++] = vcount - (lon + 2) - 1;
            indices[i++] = vcount - (lon + 1) - 1;
        }


        GeometryContext geometryContext;
        geometryContext.pVertices = attributes;
        geometryContext.pPositions = reinterpret_cast<float*>(positions);
        geometryContext.stridePositionsf32 = sizeof(float3) / sizeof(float);
        geometryContext.pNormals = reinterpret_cast<float*>(attributes) + 0u;
        geometryContext.strideNormalsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTangents = reinterpret_cast<float*>(attributes) + 3u;
        geometryContext.strideTangentsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pTexcoords = reinterpret_cast<float*>(attributes) + 7u;
        geometryContext.strideTexcoordsf32 = sizeof(Vertex_NormalTangentTexCoord) / sizeof(float);
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = BoundingBox::CenterExtents(offset, PK_FLOAT3_ONE * radius);
        auto virtualMesh = CreateMeshStatic(baseMesh, &geometryContext, "Primitive_Sphere");

        free(buffer);

        return virtualMesh;
    }
}
