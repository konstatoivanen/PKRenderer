#include "PrecompiledHeader.h"
#include <cassert>
#include <mikktspace/mikktspace.h>
#include "Core/Utilities/Memory.h"
#include "Core/CLI/Log.h"
#include "Core/Math/Extended.h"
#include "Core/Rendering/Mesh.h"
#include "MeshUtilities.h"

namespace PK::MeshUtilities
{
    namespace MikktsInterface
    {
        static int GetNumFaces(const SMikkTSpaceContext* pContext) { return static_cast<GeometryContext*>(pContext->m_pUserData)->countIndex / 3; }
        
        static int GetNumVerticesOfFace([[maybe_unused]] const SMikkTSpaceContext* pContext, [[maybe_unused]] const int iFace) { return 3; }
        
        static void GetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            auto meshData = static_cast<GeometryContext*>(pContext->m_pUserData);
            fvPosOut[0] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_POSITION.x;
            fvPosOut[1] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_POSITION.y;
            fvPosOut[2] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_POSITION.z;
        }

        static void GetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
        {
            auto meshData = static_cast<GeometryContext*>(pContext->m_pUserData);
            fvNormOut[0] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_NORMAL.x;
            fvNormOut[1] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_NORMAL.y;
            fvNormOut[2] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_NORMAL.z;
        }

        static void GetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
        {
            auto meshData = static_cast<GeometryContext*>(pContext->m_pUserData);
            fvTexcOut[0] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_TEXCOORD0.x;
            fvTexcOut[1] = meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_TEXCOORD0.y;
        }

        static void SetTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
        {
            auto meshData = static_cast<GeometryContext*>(pContext->m_pUserData);
            meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_TANGENT.x = fvTangent[0];
            meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_TANGENT.y = fvTangent[1];
            meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_TANGENT.z = fvTangent[2];
            meshData->pVertices[meshData->pIndices[iFace * 3 + iVert]].in_TANGENT.w = fSign;
        }
    }

    template<typename Ts, typename Td>
    PK_FORCE_NOINLINE static void ConvertVertices(void* dst, const void* src, size_t count, size_t strideSrc, size_t strideDst, size_t components)
    {
        for (auto i = 0ull; i < count; ++i)
        {
            const void* vsrc = reinterpret_cast<const char*>(src) + strideSrc * i;
            void* vdst = reinterpret_cast<char*>(dst) + strideDst * i;

            if constexpr (sizeof(Ts) == sizeof(Td))
            {
                memcpy(vdst, vsrc, sizeof(Ts) * components);
            }
            else
            {
                for (auto j = 0u; j < components; ++j)
                { 
                    // Filty UB here. should use memcpy instead.
                    if constexpr (TIsSame<Ts, float> && TIsSame<Td, uint16_t>)
                    {
                        reinterpret_cast<uint16_t*>(vdst)[j] = math::f32tof16(reinterpret_cast<const float*>(vsrc)[j]);
                    }
                    else if constexpr (TIsSame<Ts, uint16_t> && TIsSame<Td, float>)
                    {
                        reinterpret_cast<float*>(vdst)[j] = math::f16tof32(reinterpret_cast<const uint16_t*>(vsrc)[j]);
                    }
                    else
                    {
                        reinterpret_cast<Td*>(vdst)[j] = static_cast<Td>(reinterpret_cast<const Ts*>(vsrc)[j]);
                    }
                }
            }
        }
    }

    static void ConvertVertices(void* dst, const void* src, size_t count, size_t strideSrc, size_t strideDst, ElementType typeSrc, ElementType typeDst)
    {
        auto componentsSrc = RHIEnumConvert::Components(typeSrc);
        auto componentsDst = RHIEnumConvert::Components(typeDst);
        auto components = math::min(componentsSrc, componentsDst);
        auto scalarSrc = RHIEnumConvert::Scalar(typeSrc);
        auto scalarDst = RHIEnumConvert::Scalar(typeDst);
        auto sizeSrc = RHIEnumConvert::Size(scalarSrc);
        auto sizeDst = RHIEnumConvert::Size(scalarDst);

        if (sizeSrc == sizeDst)
        {
            ConvertVertices<char, char>(dst, src, count, strideSrc, strideDst, sizeSrc * components);
            return;
        }

        #define PK_VERTEX_CONVERT(Es, Ed, Ts, Td)\
            if (scalarSrc == Es && scalarDst == Ed) \
            { \
                ConvertVertices<Ts, Td>(dst, src, count, strideSrc, strideDst, components); \
                return; \
            }

        // Special conversions for half. conversions to/from half from other types is not supported.
        PK_VERTEX_CONVERT(ElementType::Float, ElementType::Half, float, uint16_t)
        PK_VERTEX_CONVERT(ElementType::Half, ElementType::Float, uint16_t, float)

        PK_VERTEX_CONVERT(ElementType::Ushort, ElementType::Uint,   uint16_t, uint32_t)
        PK_VERTEX_CONVERT(ElementType::Ushort, ElementType::Int,    uint16_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Ushort, ElementType::Float,  uint16_t, float)
        PK_VERTEX_CONVERT(ElementType::Ushort, ElementType::Ulong,  uint16_t, uint64_t)
        PK_VERTEX_CONVERT(ElementType::Ushort, ElementType::Long,   uint16_t, int64_t)
        PK_VERTEX_CONVERT(ElementType::Ushort, ElementType::Double, uint16_t, double)

        PK_VERTEX_CONVERT(ElementType::Short, ElementType::Uint,   int16_t, uint32_t)
        PK_VERTEX_CONVERT(ElementType::Short, ElementType::Int,    int16_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Short, ElementType::Float,  int16_t, float)
        PK_VERTEX_CONVERT(ElementType::Short, ElementType::Ulong,  int16_t, uint64_t)
        PK_VERTEX_CONVERT(ElementType::Short, ElementType::Long,   int16_t, int64_t)
        PK_VERTEX_CONVERT(ElementType::Short, ElementType::Double, int16_t, double)

        PK_VERTEX_CONVERT(ElementType::Uint, ElementType::Ushort, uint32_t, uint16_t)
        PK_VERTEX_CONVERT(ElementType::Uint, ElementType::Short,  uint32_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Uint, ElementType::Ulong,  uint32_t, uint64_t)
        PK_VERTEX_CONVERT(ElementType::Uint, ElementType::Long,   uint32_t, int64_t)
        PK_VERTEX_CONVERT(ElementType::Uint, ElementType::Double, uint32_t, double)

        PK_VERTEX_CONVERT(ElementType::Int, ElementType::Ushort, int32_t, uint16_t)
        PK_VERTEX_CONVERT(ElementType::Int, ElementType::Short,  int32_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Int, ElementType::Ulong,  int32_t, uint64_t)
        PK_VERTEX_CONVERT(ElementType::Int, ElementType::Long,   int32_t, int64_t)
        PK_VERTEX_CONVERT(ElementType::Int, ElementType::Double, int32_t, double)

        PK_VERTEX_CONVERT(ElementType::Float, ElementType::Ushort, float, uint16_t)
        PK_VERTEX_CONVERT(ElementType::Float, ElementType::Short,  float, int16_t)
        PK_VERTEX_CONVERT(ElementType::Float, ElementType::Ulong,  float, uint64_t)
        PK_VERTEX_CONVERT(ElementType::Float, ElementType::Long,   float, int64_t)
        PK_VERTEX_CONVERT(ElementType::Float, ElementType::Double, float, double)

        PK_VERTEX_CONVERT(ElementType::Ulong, ElementType::Ushort, uint64_t, uint16_t)
        PK_VERTEX_CONVERT(ElementType::Ulong, ElementType::Short,  uint64_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Ulong, ElementType::Uint,   uint64_t, uint32_t)
        PK_VERTEX_CONVERT(ElementType::Ulong, ElementType::Int,    uint64_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Ulong, ElementType::Float,  uint64_t, float)

        PK_VERTEX_CONVERT(ElementType::Long, ElementType::Ushort, int64_t, uint16_t)
        PK_VERTEX_CONVERT(ElementType::Long, ElementType::Short,  int64_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Long, ElementType::Uint,   int64_t, uint32_t)
        PK_VERTEX_CONVERT(ElementType::Long, ElementType::Int,    int64_t, int16_t)
        PK_VERTEX_CONVERT(ElementType::Long, ElementType::Float,  int64_t, float)

        PK_VERTEX_CONVERT(ElementType::Double, ElementType::Ushort, double, uint16_t)
        PK_VERTEX_CONVERT(ElementType::Double, ElementType::Short,  double, int16_t)
        PK_VERTEX_CONVERT(ElementType::Double, ElementType::Uint,   double, uint32_t)
        PK_VERTEX_CONVERT(ElementType::Double, ElementType::Int,    double, int16_t)
        PK_VERTEX_CONVERT(ElementType::Double, ElementType::Float,  double, float)

        #undef PK_VERTEX_CONVERT
    }

    void AlignVertexStreams(void* vertices, size_t count, const VertexStreamLayout& src, const VertexStreamLayout& dst)
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
                    PK_FATAL_ASSERT(vsrc.inputRate == vdst.inputRate, "Element '%s' input rate missmatch '%i' & '%i'", vdst.name.c_str(), (int)vdst.inputRate, (int)vsrc.inputRate);
                    needsAlignment |= vsrc.format != vdst.format || vsrc.stream != vdst.stream || vsrc.offset != vdst.offset || vsrc.stride != vdst.stride;
                    remap[i] = j;
                    break;
                }
            }
        }

        if (needsAlignment)
        {
            auto buffer = Memory::AllocateClear<char>(count * dst.GetStride());
            auto pverts = reinterpret_cast<char*>(vertices);

            for (auto i = 0u; i < dst.GetCount(); ++i)
            {
                const auto& vdst = dst[i];
                const auto& vsrc = src[remap[i]];
                const auto srcOffset = vsrc.stream == 0 ? 0ull : src.GetStride(vsrc.stream - 1) * count;
                const auto dstOffset = vdst.stream == 0 ? 0ull : dst.GetStride(vdst.stream - 1) * count;
                ConvertVertices(buffer + dstOffset, pverts + srcOffset, count, vsrc.stride, vdst.stride, vsrc.format, vdst.format);
            }

            Memory::Memcpy(pverts, buffer, count * dst.GetStride());
            Memory::Free(buffer);
        }
    }

    void CopyIndexBuffer(void* dst, const void* src, size_t count, size_t sizeSrc, size_t sizeDst)
    {
        if (sizeSrc == sizeDst)
        {
            memcpy(dst, src, count * sizeSrc);
            return;
        }

        if (sizeDst == sizeof(uint32_t) && sizeSrc == sizeof(uint16_t))
        {
            auto psrc = reinterpret_cast<const uint16_t*>(src);
            auto pdst = reinterpret_cast<uint32_t*>(dst);
            Memory::CopyCastArray(pdst, psrc, count);
            return;
        }

        if (sizeDst == sizeof(uint16_t) && sizeSrc == sizeof(uint32_t))
        {
            auto psrc = reinterpret_cast<const uint32_t*>(src);
            auto pdst = reinterpret_cast<uint16_t*>(dst);
            Memory::CopyCastArray(pdst, psrc, count);
            return;
        }

        PK_FATAL_ASSERT(false, "Unsupported index size");
    }

    void CalculateNormals(GeometryContext* ctx, float sign)
    {
        for (auto i = 0u; i < ctx->countIndex; i += 3u)
        {
            const auto& v0 = ctx->pVertices[ctx->pIndices[i + 0]].in_POSITION;
            const auto& v1 = ctx->pVertices[ctx->pIndices[i + 1]].in_POSITION;
            const auto& v2 = ctx->pVertices[ctx->pIndices[i + 2]].in_POSITION;
            auto& n0 = ctx->pVertices[ctx->pIndices[i + 0]].in_NORMAL;
            auto& n1 = ctx->pVertices[ctx->pIndices[i + 1]].in_NORMAL;
            auto& n2 = ctx->pVertices[ctx->pIndices[i + 2]].in_NORMAL;

            auto tangent = math::normalize(v1 - v0);
            auto binormal = math::normalize(v2 - v0);
            auto normal = math::normalize(math::cross(tangent, binormal));
            n0 += normal;
            n1 += normal;
            n2 += normal;
        }

        for (auto i = 0u; i < ctx->countVertex; ++i)
        {
            ctx->pVertices[i].in_NORMAL = math::normalize(ctx->pVertices[i].in_NORMAL) * sign;
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

        PK_FATAL_ASSERT(genTangSpaceDefault(&context), "Failed to calculate tangents");
    }


    MeshletBuildData::MeshletBuildData(size_t count_meshlet)
    {
        const auto count_index = count_meshlet * PKAssets::PK_MESHLET_MAX_TRIANGLES * 3ull;
        const auto count_vertex = count_meshlet * PKAssets::PK_MESHLET_MAX_VERTICES;

        size_t size = 0ull;
        const auto offset_indices = Memory::AlignSize<uint8_t>(size);
        size = offset_indices + sizeof(uint8_t) * count_index;
        const auto offset_vertices = Memory::AlignSize<PKAssets::PKMeshletVertex>(size);
        size = offset_vertices + sizeof(PKAssets::PKMeshletVertex) * count_vertex;
        const auto offset_meshlets = Memory::AlignSize<PKAssets::PKMeshlet>(size);
        size = offset_meshlets + sizeof(PKAssets::PKMeshlet) * count_meshlet;

        buffer = Memory::AllocateClear<uint8_t>(size);
        indices = Memory::CastOffsetPtr<uint8_t>(buffer, offset_indices);
        vertices = Memory::CastOffsetPtr<PKAssets::PKMeshletVertex>(buffer, offset_vertices);
        meshlets = Memory::CastOffsetPtr<PKAssets::PKMeshlet>(buffer, offset_meshlets);
    }

    MeshletBuildData::MeshletBuildData(MeshletBuildData&& other)
    {
        if (&other != this)
        {
            submesh = other.submesh;
            meshlet_count = other.meshlet_count;
            vertex_count = other.vertex_count;
            index_count = other.index_count;
            buffer = other.buffer;
            indices = other.indices;
            vertices = other.vertices;
            meshlets = other.meshlets;
            other.buffer = nullptr;
        }
    }

    MeshletBuildData::~MeshletBuildData()
    {
        Memory::Free(buffer);
        buffer = nullptr;
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

        assert(ctx->countIndex % 3 == 0);

        const auto max_vertices = PKAssets::PK_MESHLET_MAX_VERTICES;
        const auto max_triangles = PKAssets::PK_MESHLET_MAX_TRIANGLES;
        const auto max_vertices_conservative = PKAssets::PK_MESHLET_MAX_VERTICES - 2;

        auto meshlet_limit_vertices = (ctx->countIndex + max_vertices_conservative - 1) / max_vertices_conservative;
        auto meshlet_limit_triangles = (ctx->countIndex / 3 + PKAssets::PK_MESHLET_MAX_TRIANGLES - 1) / PKAssets::PK_MESHLET_MAX_TRIANGLES;
        auto max_meshlets = meshlet_limit_vertices > meshlet_limit_triangles ? meshlet_limit_vertices : meshlet_limit_triangles;

        MeshletBuildData output(max_meshlets);

        auto* meshlet_vertices = Memory::Allocate<uint32_t>(max_meshlets * PKAssets::PK_MESHLET_MAX_VERTICES);
        auto* meshlet_indices = output.indices;
        auto* used = Memory::Allocate<uint8_t>(ctx->countVertex);
        
        memset(used, -1, ctx->countVertex);

        memcpy(output.submesh.bbmin, &ctx->aabb.min.x, sizeof(float3));
        memcpy(output.submesh.bbmax, &ctx->aabb.max.x, sizeof(float3));

        auto pack_meshlet_vertex = [](GeometryContext* ctx, PKAssets::PKMeshletVertex* out_vertex, uint32_t index)
        {
            *out_vertex = PKAssets::PackPKMeshletVertex(
                &ctx->pVertices[index].in_POSITION.x,
                &ctx->pVertices[index].in_TEXCOORD0.x,
                &ctx->pVertices[index].in_NORMAL.x,
                &ctx->pVertices[index].in_TANGENT.x,
                nullptr,
                &ctx->aabb.min.x,
                &ctx->aabb.max.x);
        };

        auto pack_meshlet = [](GeometryContext* ctx, PKAssets::PKMeshlet* out_meshlet, MeshletIndexInfo meshlet, const uint32_t* meshlet_vertices, const uint8_t* meshlet_indices)
        {
            float3 center = PK_FLOAT3_ZERO, extents = PK_FLOAT3_ZERO, cone_apex = PK_FLOAT3_ZERO;
            sbyte3 cone_axis_s8{};
            sbyte cone_cutoff_s8 = 0;

            // Compute bounds
            {
                const auto index_count = meshlet.triangle_count * 3;
                float3 normals[PKAssets::PK_MESHLET_MAX_TRIANGLES * 3];
                float3 corners[PKAssets::PK_MESHLET_MAX_TRIANGLES * 3][3];
                uint32_t valid_tri_count = 0u;

                for (auto i = 0u; i < index_count; i += 3u)
                {
                    bool isValid = false;
                    const auto a = &ctx->pVertices[meshlet_vertices[meshlet.vertex_offset + meshlet_indices[meshlet.triangle_offset + i + 0]]].in_POSITION.x;
                    const auto b = &ctx->pVertices[meshlet_vertices[meshlet.vertex_offset + meshlet_indices[meshlet.triangle_offset + i + 1]]].in_POSITION.x;
                    const auto c = &ctx->pVertices[meshlet_vertices[meshlet.vertex_offset + meshlet_indices[meshlet.triangle_offset + i + 2]]].in_POSITION.x;
                    normals[valid_tri_count] = math::triangleNormal(a, b, c, isValid);
                    corners[valid_tri_count][0] = float3(a);
                    corners[valid_tri_count][1] = float3(b);
                    corners[valid_tri_count][2] = float3(c);
                    valid_tri_count += (uint32_t)isValid;
                }

                // degenerate cluster, no valid triangles => trivial reject (cone data is 0)
                if (valid_tri_count > 0u)
                {
                    const auto psphere = math::pointsToSphere(corners[0], valid_tri_count * 3u);
                    const auto nsphere = math::pointsToSphere(normals, valid_tri_count);
                    const auto axis = math::safenormalize(nsphere.xyz());
                    auto aabb = math::pointsToAABB(corners[0], valid_tri_count * 3u);
                    center = aabb.center();
                    extents = aabb.extents();
                    cone_cutoff_s8 = 127;

                    auto minCosA = 1.0f;

                    for (auto i = 0u; i < valid_tri_count; ++i)
                    {
                        minCosA = math::min(minCosA, math::dot(normals[i], axis));
                    }

                    if (minCosA > 0.1f)
                    {
                        auto maxt = 0.0f;

                        for (auto i = 0u; i < valid_tri_count; ++i)
                        {
                            maxt = math::max(maxt, math::dot(psphere.xyz - corners[i][0], normals[i]) / math::dot(axis, normals[i]));
                        }

                        cone_apex = (psphere.xyz - axis) * maxt;
                        cone_axis_s8 = math::packSnorm8(axis);
                        const auto cone_axis_s8_e = math::abs(float3(cone_axis_s8) / 127.0f - axis);
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
                &cone_axis_s8.x,
                cone_cutoff_s8,
                &cone_apex.x,
                &center.x,
                &extents.x,
                &center.x,
                -1.0f,
                &center.x,
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
        output.index_count = 12u * ((meshlet.triangle_offset + meshlet.triangle_count * 3u + 11u) / 12u);
        output.vertex_count = meshlet.vertex_offset + meshlet.vertex_count;
        output.submesh.firstMeshlet = 0u;
        output.submesh.meshletCount = output.meshlet_count;

        Memory::Free(meshlet_vertices);
        Memory::Free(used);

        return output;
    }

    MeshStatic CreateMeshStatic(MeshStaticAllocator* allocator, GeometryContext* ctx, const char* name)
    {
        CalculateTangents(ctx);

        auto meshlets = BuildMeshletsMonotone(ctx);

        SubMesh submesh;
        submesh.name = 0u;
        submesh.vertexFirst = 0u;
        submesh.vertexCount = ctx->countVertex;
        submesh.indexFirst = 0u;
        submesh.indexCount = ctx->countIndex;
        submesh.meshletFirst = 0u;
        submesh.meshletCount = 0u;
        submesh.bounds = ctx->aabb;

        MeshStaticDescriptor desc{};
        desc.name = name;
        desc.regular.pVertices = ctx->pVertices;
        desc.regular.pIndices = ctx->pIndices;
        desc.regular.pSubmeshes = &submesh;
        desc.regular.indexSize = sizeof(uint32_t);
        desc.regular.vertexCount = ctx->countVertex;
        desc.regular.indexCount = ctx->countIndex;
        desc.regular.submeshCount = 1u;
        desc.regular.streamLayout =
        {
            // VertexDefault layout. allocator will rearrange & pack vertices if needed.
            { ElementType::Float3, PK_RHI_VS_POSITION, 0 },
            { ElementType::Float3, PK_RHI_VS_NORMAL, 0 },
            { ElementType::Float2, PK_RHI_VS_TEXCOORD0, 0 },
            { ElementType::Float4, PK_RHI_VS_TANGENT, 0 },
        };
        desc.meshlets.pSubmeshes = &meshlets.submesh;
        desc.meshlets.submeshCount = 1u;
        desc.meshlets.pMeshlets = meshlets.meshlets;
        desc.meshlets.meshletCount = meshlets.meshlet_count;
        desc.meshlets.pVertices = meshlets.vertices;
        desc.meshlets.vertexCount = meshlets.vertex_count;
        desc.meshlets.pIndices = meshlets.indices;
        desc.meshlets.triangleCount = meshlets.index_count / 3u;

        MeshStatic mesh(allocator, desc);
        return mesh;
    }

    MeshStatic CreateBoxMeshStatic(MeshStaticAllocator* allocator, const float3& offset, const float3& extents)
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

        VertexDefault vertices[vcount];
        vertices[0] = { p0,down, uv11, float4(front, 1) };
        vertices[1] = { p1,down, uv01, float4(front, 1) };
        vertices[2] = { p2,down, uv00, float4(front, 1) };
        vertices[3] = { p3,down, uv10, float4(front, 1) };
        vertices[4] = { p7,left, uv11, float4(down, 1) };
        vertices[5] = { p4,left, uv01, float4(down, 1) };
        vertices[6] = { p0,left, uv00, float4(down, 1) };
        vertices[7] = { p3,left, uv10, float4(down, 1) };
        vertices[8] = { p4,front, uv11, float4(down, 1) };
        vertices[9] = { p5,front, uv01, float4(down, 1) };
        vertices[10] = { p1, front, uv00, float4(down, 1) };
        vertices[11] = { p0, front, uv10, float4(down, 1) };
        vertices[12] = { p6, back, uv11, float4(up, 1) };
        vertices[13] = { p7, back, uv01, float4(up, 1) };
        vertices[14] = { p3, back, uv00, float4(up, 1) };
        vertices[15] = { p2, back, uv10, float4(up, 1) };
        vertices[16] = { p5, right, uv11, float4(up, 1) };
        vertices[17] = { p6, right, uv01, float4(up, 1) };
        vertices[18] = { p2, right, uv00, float4(up, 1) };
        vertices[19] = { p1, right, uv10, float4(up, 1) };
        vertices[20] = { p7, up, uv11, float4(back, 1) };
        vertices[21] = { p6, up, uv01, float4(back, 1) };
        vertices[22] = { p5, up, uv00, float4(back, 1) };
        vertices[23] = { p4, up, uv10, float4(back, 1) };

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
        geometryContext.pVertices = vertices;
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = math::centerExtentsToAABB(offset, extents);
        return CreateMeshStatic(allocator, &geometryContext, "Primitive_Box");
    }

    MeshStatic CreateQuadMeshStatic(MeshStaticAllocator* allocator, const float2& min, const float2& max)
    {
        constexpr auto vcount = 4u;
        constexpr auto icount = 2u;
        VertexDefault vertices[vcount];
        vertices[0] = { { min.x, min.y, 0.0f }, PK_FLOAT3_UP, { 0.0f, 0.0f }, float4(PK_FLOAT3_BACKWARD, 1) };
        vertices[1] = { { min.x, max.y, 0.0f }, PK_FLOAT3_UP, { 0.0f, 1.0f }, float4(PK_FLOAT3_BACKWARD, 1) };
        vertices[2] = { { max.x, max.y, 0.0f }, PK_FLOAT3_UP, { 1.0f, 1.0f }, float4(PK_FLOAT3_BACKWARD, 1) };
        vertices[3] = { { max.x, min.y, 0.0f }, PK_FLOAT3_UP, { 1.0f, 0.0f }, float4(PK_FLOAT3_BACKWARD, 1) };
        uint32_t indices[] = { 0u,1u,2u, 2u,3u,0u };

        GeometryContext geometryContext;
        geometryContext.pVertices = vertices;
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = AABB<float3>({ min.x, min.y, 0.0f }, { max.x, max.y, 0.0f });
        return CreateMeshStatic(allocator, &geometryContext, "Primitive_Box");
    }

    MeshStatic CreatePlaneMeshStatic(MeshStaticAllocator* allocator, const float2 & center, const float2 & extents, uint2 resolution)
    {
        auto vcount = resolution.x * resolution.y * 4;
        auto icount = resolution.x * resolution.y * 6;
        auto bufferSize = sizeof(VertexDefault) * vcount + sizeof(uint32_t) * icount;
        auto* buffer = Memory::AllocateAligned(bufferSize);
        auto* vertices = reinterpret_cast<VertexDefault*>(buffer);
        auto* indices = reinterpret_cast<uint32_t*>(vertices + vcount);

        auto isize = float3(extents.x / resolution.x, extents.y / resolution.y, 0.0f) * 2.0f;
        auto min = float3(center - extents, 0);

        for (auto x = 0u; x < resolution.x; ++x)
        for (auto y = 0u; y < resolution.y; ++y)
        {
            auto vmin = min + isize * float3(x, y, 0);
            auto baseVertex = (y * resolution.x + x) * 4;
            auto baseIndex = (y * resolution.x + x) * 6;

            vertices[baseVertex + 0] = { { vmin + isize.zzz }, PK_FLOAT3_BACKWARD, float2(0, 0), PK_FLOAT4_ZERO };
            vertices[baseVertex + 1] = { { vmin + isize.zyz }, PK_FLOAT3_BACKWARD, float2(0, 1), PK_FLOAT4_ZERO };
            vertices[baseVertex + 2] = { { vmin + isize.xyz }, PK_FLOAT3_BACKWARD, float2(1, 1), PK_FLOAT4_ZERO };
            vertices[baseVertex + 3] = { { vmin + isize.xzz }, PK_FLOAT3_BACKWARD, float2(1, 0), PK_FLOAT4_ZERO };

            indices[baseIndex + 0] = baseVertex + 0;
            indices[baseIndex + 1] = baseVertex + 1;
            indices[baseIndex + 2] = baseVertex + 2;

            indices[baseIndex + 3] = baseVertex + 2;
            indices[baseIndex + 4] = baseVertex + 3;
            indices[baseIndex + 5] = baseVertex + 0;
        }

        GeometryContext geometryContext;
        geometryContext.pVertices = vertices;
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = math::centerExtentsToAABB(float3(center.xy, 0.0f), float3(extents.xy, 0.0f));
        auto virtualMesh = CreateMeshStatic(allocator, &geometryContext, "Primitive_Plane"); 
        Memory::Free(buffer);
        return virtualMesh;
    }

    MeshStatic CreateSphereMeshStatic(MeshStaticAllocator* allocator, const float3& offset, const float radius)
    {
        const auto longc = 24;
        const auto lattc = 16;
        const auto vcount = (longc + 1) * lattc + 2;
        const auto icount = ((lattc - 1) * longc * 2 + longc * 2) * 3;

        //Vertex_Full
        auto* buffer = Memory::AllocateAligned(sizeof(VertexDefault) * vcount + sizeof(uint) * icount);
        auto* vertices = reinterpret_cast<VertexDefault*>(buffer);
        auto* indices = reinterpret_cast<uint*>(vertices + vcount);

        vertices[0].in_POSITION = PK_FLOAT3_UP * radius;

        for (auto lat = 0u; lat < lattc; lat++)
        {
            float a1 = PK_FLOAT_PI * (float)(lat + 1) / (lattc + 1);
            float sin1 = math::sin(a1);
            float cos1 = math::cos(a1);

            for (auto lon = 0u; lon <= longc; lon++)
            {
                float a2 = PK_FLOAT_TWO_PI * (float)(lon == longc ? 0 : lon) / longc;
                float sin2 = math::sin(a2);
                float cos2 = math::cos(a2);
                vertices[lon + lat * (longc + 1) + 1].in_POSITION = float3(sin1 * cos2, cos1, sin1 * sin2) * radius;
            }
        }

        vertices[vcount - 1].in_POSITION = PK_FLOAT3_UP * -radius;

        for (int n = 0; n < vcount; ++n)
        {
            vertices[n].in_NORMAL = math::normalize(vertices[n].in_POSITION);
        }

        vertices[0].in_TEXCOORD0 = PK_FLOAT2_UP;
        vertices[vcount - 1].in_TEXCOORD0 = PK_FLOAT2_ZERO;

        for (auto lat = 0u; lat < lattc; lat++)
        {
            for (int lon = 0u; lon <= longc; lon++)
            {
                vertices[lon + lat * (longc + 1) + 1].in_TEXCOORD0 = float2((float)lon / longc, 1.0f - (float)(lat + 1) / (lattc + 1));
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
        geometryContext.pVertices = vertices;
        geometryContext.pIndices = indices;
        geometryContext.countVertex = vcount;
        geometryContext.countIndex = icount;
        geometryContext.aabb = math::centerExtentsToAABB(offset, PK_FLOAT3_ONE * radius);
        auto virtualMesh = CreateMeshStatic(allocator, &geometryContext, "Primitive_Sphere");
        Memory::Free(buffer);
        return virtualMesh;
    }
}
