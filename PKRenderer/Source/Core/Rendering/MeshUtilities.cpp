#include "PrecompiledHeader.h"
#include <mikktspace/mikktspace.h>
#include "Core/CLI/Log.h"
#include "Core/Rendering/MeshStaticCollection.h"
#include "Core/Rendering/MeshStaticAsset.h"
#include "Core/Rendering/Mesh.h"
#include "MeshUtilities.h"

namespace PK::MeshUtilities
{
    // Zeux meshoptimizer code insert here. Workaround for now.
    // @TODO implement your own stuff. 
    namespace zeux
    {
        struct meshopt_Meshlet
        {
            /* offsets within meshlet_vertices and meshlet_triangles arrays with meshlet data */
            unsigned int vertex_offset;
            unsigned int triangle_offset;

            /* number of vertices and triangles used in the meshlet; data is stored in consecutive range defined by offset and count */
            unsigned int vertex_count;
            unsigned int triangle_count;
        };

        struct meshopt_Bounds
        {
            /* bounding sphere, useful for frustum and occlusion culling */
            float center[3];
            float radius;

            /* normal cone, useful for backface culling */
            float cone_apex[3];
            float cone_axis[3];
            float cone_cutoff; /* = cos(angle/2) */

            /* normal cone axis and cutoff, stored in 8-bit SNORM format; decode using x/127.0 */
            signed char cone_axis_s8[3];
            signed char cone_cutoff_s8;
        };

        // This must be <= 255 since index 0xff is used internally to indice a vertex that doesn't belong to a meshlet
        const size_t kMeshletMaxVertices = 255;

        // A reasonable limit is around 2*max_vertices or less
        const size_t kMeshletMaxTriangles = 512;

        static void finishMeshlet(meshopt_Meshlet& meshlet, unsigned char* meshlet_triangles)
        {
            size_t offset = meshlet.triangle_offset + meshlet.triangle_count * 3;

            // fill 4b padding with 0
            while (offset & 3)
                meshlet_triangles[offset++] = 0;
        }

        static bool appendMeshlet(meshopt_Meshlet& meshlet, unsigned int a, unsigned int b, unsigned int c, unsigned char* used, meshopt_Meshlet* meshlets, unsigned int* meshlet_vertices, unsigned char* meshlet_triangles, size_t meshlet_offset, size_t max_vertices, size_t max_triangles)
        {
            unsigned char& av = used[a];
            unsigned char& bv = used[b];
            unsigned char& cv = used[c];

            bool result = false;

            unsigned int used_extra = (av == 0xff) + (bv == 0xff) + (cv == 0xff);

            if (meshlet.vertex_count + used_extra > max_vertices || meshlet.triangle_count >= max_triangles)
            {
                meshlets[meshlet_offset] = meshlet;

                for (size_t j = 0; j < meshlet.vertex_count; ++j)
                    used[meshlet_vertices[meshlet.vertex_offset + j]] = 0xff;

                finishMeshlet(meshlet, meshlet_triangles);

                meshlet.vertex_offset += meshlet.vertex_count;
                meshlet.triangle_offset += (meshlet.triangle_count * 3 + 3) & ~3; // 4b padding
                meshlet.vertex_count = 0;
                meshlet.triangle_count = 0;

                result = true;
            }

            if (av == 0xff)
            {
                av = (unsigned char)meshlet.vertex_count;
                meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = a;
            }

            if (bv == 0xff)
            {
                bv = (unsigned char)meshlet.vertex_count;
                meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = b;
            }

            if (cv == 0xff)
            {
                cv = (unsigned char)meshlet.vertex_count;
                meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = c;
            }

            meshlet_triangles[meshlet.triangle_offset + meshlet.triangle_count * 3 + 0] = av;
            meshlet_triangles[meshlet.triangle_offset + meshlet.triangle_count * 3 + 1] = bv;
            meshlet_triangles[meshlet.triangle_offset + meshlet.triangle_count * 3 + 2] = cv;
            meshlet.triangle_count++;

            return result;
        }

        static void computeBoundingSphere(float result[4], const float points[][3], size_t count)
        {
            assert(count > 0);

            // find extremum points along all 3 axes; for each axis we get a pair of points with min/max coordinates
            size_t pmin[3] = { 0, 0, 0 };
            size_t pmax[3] = { 0, 0, 0 };

            for (size_t i = 0; i < count; ++i)
            {
                const float* p = points[i];

                for (int axis = 0; axis < 3; ++axis)
                {
                    pmin[axis] = (p[axis] < points[pmin[axis]][axis]) ? i : pmin[axis];
                    pmax[axis] = (p[axis] > points[pmax[axis]][axis]) ? i : pmax[axis];
                }
            }

            // find the pair of points with largest distance
            float paxisd2 = 0;
            int paxis = 0;

            for (int axis = 0; axis < 3; ++axis)
            {
                const float* p1 = points[pmin[axis]];
                const float* p2 = points[pmax[axis]];

                float d2 = (p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]) + (p2[2] - p1[2]) * (p2[2] - p1[2]);

                if (d2 > paxisd2)
                {
                    paxisd2 = d2;
                    paxis = axis;
                }
            }

            // use the longest segment as the initial sphere diameter
            const float* p1 = points[pmin[paxis]];
            const float* p2 = points[pmax[paxis]];

            float center[3] = { (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2, (p1[2] + p2[2]) / 2 };
            float radius = sqrtf(paxisd2) / 2;

            // iteratively adjust the sphere up until all points fit
            for (size_t i = 0; i < count; ++i)
            {
                const float* p = points[i];
                float d2 = (p[0] - center[0]) * (p[0] - center[0]) + (p[1] - center[1]) * (p[1] - center[1]) + (p[2] - center[2]) * (p[2] - center[2]);

                if (d2 > radius * radius)
                {
                    float d = sqrtf(d2);
                    assert(d > 0);

                    float k = 0.5f + (radius / d) / 2;

                    center[0] = center[0] * k + p[0] * (1 - k);
                    center[1] = center[1] * k + p[1] * (1 - k);
                    center[2] = center[2] * k + p[2] * (1 - k);
                    radius = (radius + d) / 2;
                }
            }

            result[0] = center[0];
            result[1] = center[1];
            result[2] = center[2];
            result[3] = radius;
        }

        inline int meshopt_quantizeSnorm(float v, int N)
        {
            const float scale = float((1 << (N - 1)) - 1);

            float round = (v >= 0 ? 0.5f : -0.5f);

            v = (v >= -1) ? v : -1;
            v = (v <= +1) ? v : +1;

            return int(v * scale + round);
        }

        size_t meshopt_buildMeshletsBound(size_t index_count, size_t max_vertices, size_t max_triangles)
        {
            assert(index_count % 3 == 0);
            assert(max_vertices >= 3 && max_vertices <= kMeshletMaxVertices);
            assert(max_triangles >= 1 && max_triangles <= kMeshletMaxTriangles);
            assert(max_triangles % 4 == 0); // ensures the caller will compute output space properly as index data is 4b aligned

            (void)kMeshletMaxVertices;
            (void)kMeshletMaxTriangles;

            // meshlet construction is limited by max vertices and max triangles per meshlet
            // the worst case is that the input is an unindexed stream since this equally stresses both limits
            // note that we assume that in the worst case, we leave 2 vertices unpacked in each meshlet - if we have space for 3 we can pack any triangle
            size_t max_vertices_conservative = max_vertices - 2;
            size_t meshlet_limit_vertices = (index_count + max_vertices_conservative - 1) / max_vertices_conservative;
            size_t meshlet_limit_triangles = (index_count / 3 + max_triangles - 1) / max_triangles;

            return meshlet_limit_vertices > meshlet_limit_triangles ? meshlet_limit_vertices : meshlet_limit_triangles;
        }

        size_t meshopt_buildMeshletsScan(meshopt_Meshlet* meshlets, unsigned int* meshlet_vertices, unsigned char* meshlet_triangles, const unsigned int* indices, size_t index_count, size_t vertex_count, size_t max_vertices, size_t max_triangles)
        {
            assert(index_count % 3 == 0);

            assert(max_vertices >= 3 && max_vertices <= kMeshletMaxVertices);
            assert(max_triangles >= 1 && max_triangles <= kMeshletMaxTriangles);
            assert(max_triangles % 4 == 0); // ensures the caller will compute output space properly as index data is 4b aligned

            // index of the vertex in the meshlet, 0xff if the vertex isn't used
            unsigned char* used = reinterpret_cast<unsigned char*>(malloc(vertex_count));
            memset(used, -1, vertex_count);

            meshopt_Meshlet meshlet = {};
            size_t meshlet_offset = 0;

            for (size_t i = 0; i < index_count; i += 3)
            {
                unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
                assert(a < vertex_count&& b < vertex_count&& c < vertex_count);

                // appends triangle to the meshlet and writes previous meshlet to the output if full
                meshlet_offset += appendMeshlet(meshlet, a, b, c, used, meshlets, meshlet_vertices, meshlet_triangles, meshlet_offset, max_vertices, max_triangles);
            }

            if (meshlet.triangle_count)
            {
                finishMeshlet(meshlet, meshlet_triangles);

                meshlets[meshlet_offset++] = meshlet;
            }

            assert(meshlet_offset <= meshopt_buildMeshletsBound(index_count, max_vertices, max_triangles));

            free(used);

            return meshlet_offset;
        }

        meshopt_Bounds meshopt_computeClusterBounds(const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
        {
            assert(index_count % 3 == 0);
            assert(index_count / 3 <= kMeshletMaxTriangles);
            assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
            assert(vertex_positions_stride % sizeof(float) == 0);

            (void)vertex_count;

            size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

            // compute triangle normals and gather triangle corners
            float normals[kMeshletMaxTriangles][3];
            float corners[kMeshletMaxTriangles][3][3];
            size_t triangles = 0;

            for (size_t i = 0; i < index_count; i += 3)
            {
                unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
                assert(a < vertex_count&& b < vertex_count&& c < vertex_count);

                const float* p0 = vertex_positions + vertex_stride_float * a;
                const float* p1 = vertex_positions + vertex_stride_float * b;
                const float* p2 = vertex_positions + vertex_stride_float * c;

                float p10[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
                float p20[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };

                float normalx = p10[1] * p20[2] - p10[2] * p20[1];
                float normaly = p10[2] * p20[0] - p10[0] * p20[2];
                float normalz = p10[0] * p20[1] - p10[1] * p20[0];

                float area = sqrtf(normalx * normalx + normaly * normaly + normalz * normalz);

                // no need to include degenerate triangles - they will be invisible anyway
                if (area == 0.f)
                    continue;

                // record triangle normals & corners for future use; normal and corner 0 define a plane equation
                normals[triangles][0] = normalx / area;
                normals[triangles][1] = normaly / area;
                normals[triangles][2] = normalz / area;
                memcpy(corners[triangles][0], p0, 3 * sizeof(float));
                memcpy(corners[triangles][1], p1, 3 * sizeof(float));
                memcpy(corners[triangles][2], p2, 3 * sizeof(float));
                triangles++;
            }

            meshopt_Bounds bounds = {};

            // degenerate cluster, no valid triangles => trivial reject (cone data is 0)
            if (triangles == 0)
                return bounds;

            // compute cluster bounding sphere; we'll use the center to determine normal cone apex as well
            float psphere[4] = {};
            computeBoundingSphere(psphere, corners[0], triangles * 3);

            float center[3] = { psphere[0], psphere[1], psphere[2] };

            // treating triangle normals as points, find the bounding sphere - the sphere center determines the optimal cone axis
            float nsphere[4] = {};
            computeBoundingSphere(nsphere, normals, triangles);

            float axis[3] = { nsphere[0], nsphere[1], nsphere[2] };
            float axislength = sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
            float invaxislength = axislength == 0.f ? 0.f : 1.f / axislength;

            axis[0] *= invaxislength;
            axis[1] *= invaxislength;
            axis[2] *= invaxislength;

            // compute a tight cone around all normals, mindp = cos(angle/2)
            float mindp = 1.f;

            for (size_t i = 0; i < triangles; ++i)
            {
                float dp = normals[i][0] * axis[0] + normals[i][1] * axis[1] + normals[i][2] * axis[2];

                mindp = (dp < mindp) ? dp : mindp;
            }

            // fill bounding sphere info; note that below we can return bounds without cone information for degenerate cones
            bounds.center[0] = center[0];
            bounds.center[1] = center[1];
            bounds.center[2] = center[2];
            bounds.radius = psphere[3];

            // degenerate cluster, normal cone is larger than a hemisphere => trivial accept
            // note that if mindp is positive but close to 0, the triangle intersection code below gets less stable
            // we arbitrarily decide that if a normal cone is ~168 degrees wide or more, the cone isn't useful
            if (mindp <= 0.1f)
            {
                bounds.cone_cutoff = 1;
                bounds.cone_cutoff_s8 = 127;
                return bounds;
            }

            float maxt = 0;

            // we need to find the point on center-t*axis ray that lies in negative half-space of all triangles
            for (size_t i = 0; i < triangles; ++i)
            {
                // dot(center-t*axis-corner, trinormal) = 0
                // dot(center-corner, trinormal) - t * dot(axis, trinormal) = 0
                float cx = center[0] - corners[i][0][0];
                float cy = center[1] - corners[i][0][1];
                float cz = center[2] - corners[i][0][2];

                float dc = cx * normals[i][0] + cy * normals[i][1] + cz * normals[i][2];
                float dn = axis[0] * normals[i][0] + axis[1] * normals[i][1] + axis[2] * normals[i][2];

                // dn should be larger than mindp cutoff above
                assert(dn > 0.f);
                float t = dc / dn;

                maxt = (t > maxt) ? t : maxt;
            }

            // cone apex should be in the negative half-space of all cluster triangles by construction
            bounds.cone_apex[0] = center[0] - axis[0] * maxt;
            bounds.cone_apex[1] = center[1] - axis[1] * maxt;
            bounds.cone_apex[2] = center[2] - axis[2] * maxt;

            // note: this axis is the axis of the normal cone, but our test for perspective camera effectively negates the axis
            bounds.cone_axis[0] = axis[0];
            bounds.cone_axis[1] = axis[1];
            bounds.cone_axis[2] = axis[2];

            // cos(a) for normal cone is mindp; we need to add 90 degrees on both sides and invert the cone
            // which gives us -cos(a+90) = -(-sin(a)) = sin(a) = sqrt(1 - cos^2(a))
            bounds.cone_cutoff = sqrtf(1 - mindp * mindp);

            // quantize axis & cutoff to 8-bit SNORM format
            bounds.cone_axis_s8[0] = (signed char)(meshopt_quantizeSnorm(bounds.cone_axis[0], 8));
            bounds.cone_axis_s8[1] = (signed char)(meshopt_quantizeSnorm(bounds.cone_axis[1], 8));
            bounds.cone_axis_s8[2] = (signed char)(meshopt_quantizeSnorm(bounds.cone_axis[2], 8));

            // for the 8-bit test to be conservative, we need to adjust the cutoff by measuring the max. error
            float cone_axis_s8_e0 = fabsf(bounds.cone_axis_s8[0] / 127.f - bounds.cone_axis[0]);
            float cone_axis_s8_e1 = fabsf(bounds.cone_axis_s8[1] / 127.f - bounds.cone_axis[1]);
            float cone_axis_s8_e2 = fabsf(bounds.cone_axis_s8[2] / 127.f - bounds.cone_axis[2]);

            // note that we need to round this up instead of rounding to nearest, hence +1
            int cone_cutoff_s8 = int(127 * (bounds.cone_cutoff + cone_axis_s8_e0 + cone_axis_s8_e1 + cone_axis_s8_e2) + 1);

            bounds.cone_cutoff_s8 = (cone_cutoff_s8 > 127) ? 127 : (signed char)(cone_cutoff_s8);

            return bounds;
        }

        meshopt_Bounds meshopt_computeMeshletBounds(const unsigned int* meshlet_vertices, const unsigned char* meshlet_triangles, size_t triangle_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
        {
            assert(triangle_count <= kMeshletMaxTriangles);
            assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
            assert(vertex_positions_stride % sizeof(float) == 0);

            unsigned int indices[kMeshletMaxTriangles * 3];

            for (size_t i = 0; i < triangle_count * 3; ++i)
            {
                unsigned int index = meshlet_vertices[meshlet_triangles[i]];
                assert(index < vertex_count);

                indices[i] = index;
            }

            return meshopt_computeClusterBounds(indices, triangle_count * 3, vertex_positions, vertex_count, vertex_positions_stride);
        }
    }

    namespace MikktsInterface
    {
        int GetNumFaces(const SMikkTSpaceContext* pContext) { return reinterpret_cast<GeometryContext*>(pContext->m_pUserData)->countIndex / 3; }
        
        int GetNumVerticesOfFace([[maybe_unused]] const SMikkTSpaceContext* pContext, [[maybe_unused]] const int iFace) { return 3; }
        
        void GetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<GeometryContext*>(pContext->m_pUserData);
            auto pPosition =  meshData->pPositions + meshData->pIndices[iFace * 3 + iVert] * meshData->stridePositionsf32;
            fvPosOut[0] = pPosition[0];
            fvPosOut[1] = pPosition[1];
            fvPosOut[2] = pPosition[2];
        }

        void GetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<GeometryContext*>(pContext->m_pUserData);
            auto pNormal = meshData->pNormals + meshData->pIndices[iFace * 3 + iVert] * meshData->strideNormalsf32;
            fvNormOut[0] = pNormal[0];
            fvNormOut[1] = pNormal[1];
            fvNormOut[2] = pNormal[2];
        }

        void GetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
        {
            auto meshData = reinterpret_cast<GeometryContext*>(pContext->m_pUserData);
            auto pTexcoord = meshData->pTexcoords + meshData->pIndices[iFace * 3 + iVert] * meshData->strideTexcoordsf32;
            fvTexcOut[0] = pTexcoord[0];
            fvTexcOut[1] = pTexcoord[1];
        }

        void SetTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
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

        // @TODO nasty double loop due to simple arrays
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

    static void CalculateMeshletCenterExtents(const float* positions,
        const uint32_t* vertexIndices,
        uint32_t vertexStridef32,
        uint32_t vertexFirst,
        uint32_t vertexCount,
        float* center,
        float* extents)
    {
        float bbmin[3];
        float bbmax[3];
        bbmin[0] = std::numeric_limits<float>().max();
        bbmin[1] = std::numeric_limits<float>().max();
        bbmin[2] = std::numeric_limits<float>().max();
        bbmax[0] = -std::numeric_limits<float>().max();
        bbmax[1] = -std::numeric_limits<float>().max();
        bbmax[2] = -std::numeric_limits<float>().max();

        for (auto i = 0u; i < vertexCount; ++i)
        {
            auto vertexIndex = vertexIndices[vertexFirst + i];
            auto pPosition = positions + vertexIndex * vertexStridef32;

            for (auto j = 0u; j < 3u; ++j)
            {
                if (pPosition[j] < bbmin[j])
                {
                    bbmin[j] = pPosition[j];
                }

                if (pPosition[j] > bbmax[j])
                {
                    bbmax[j] = pPosition[j];
                }
            }
        }

        for (auto i = 0u; i < 3; ++i)
        {
            center[i] = bbmin[i] + (bbmax[i] - bbmin[i]) * 0.5f;
            extents[i] = (bbmax[i] - bbmin[i]) * 0.5f;
        }
    }

    MeshletBuildData BuildMeshletsMonotone(GeometryContext* ctx)
    {
        MeshletBuildData output{};

        size_t max_meshlets = zeux::meshopt_buildMeshletsBound(ctx->countIndex, PKAssets::PK_MESHLET_MAX_VERTICES, PKAssets::PK_MESHLET_MAX_TRIANGLES);
        std::vector<zeux::meshopt_Meshlet> meshlets(max_meshlets);
        std::vector<unsigned int> meshlet_vertices(max_meshlets * PKAssets::PK_MESHLET_MAX_VERTICES);
        std::vector<unsigned char> meshlet_triangles(max_meshlets * PKAssets::PK_MESHLET_MAX_TRIANGLES * 3);

        size_t meshlet_count = zeux::meshopt_buildMeshletsScan
        (
            meshlets.data(),
            meshlet_vertices.data(),
            meshlet_triangles.data(),
            ctx->pIndices,
            ctx->countIndex,
            ctx->countVertex,
            PKAssets::PK_MESHLET_MAX_VERTICES,
            PKAssets::PK_MESHLET_MAX_TRIANGLES
        );

        output.submesh.firstMeshlet = 0u;
        output.submesh.meshletCount = (uint32_t)meshlet_count;
        output.submesh.bbmin[0] = ctx->aabb.min[0];
        output.submesh.bbmin[1] = ctx->aabb.min[1];
        output.submesh.bbmin[2] = ctx->aabb.min[2];
        output.submesh.bbmax[0] = ctx->aabb.max[0];
        output.submesh.bbmax[1] = ctx->aabb.max[1];
        output.submesh.bbmax[2] = ctx->aabb.max[2];

        for (auto i = 0u; i < meshlet_count; ++i)
        {
            const auto& meshlet = meshlets.at(i);

            auto bounds = zeux::meshopt_computeMeshletBounds
            (
                meshlet_vertices.data() + meshlet.vertex_offset,
                meshlet_triangles.data() + meshlet.triangle_offset,
                meshlet.triangle_count,
                ctx->pPositions,
                ctx->countVertex,
                ctx->stridePositionsf32 * sizeof(float)
            );

            auto indicesOffset = output.indices.size();
            auto triangleOffset = indicesOffset / 3ull;
            auto verticesOffset = output.vertices.size();

            float center[3];
            float extents[3];
            CalculateMeshletCenterExtents(ctx->pPositions, meshlet_vertices.data(), ctx->stridePositionsf32, meshlet.vertex_offset, meshlet.vertex_count, center, extents);

            PKAssets::PKMeshlet pkmeshlet = PKAssets::PackPKMeshlet
            (
                (uint32_t)verticesOffset,
                (uint32_t)triangleOffset,
                meshlet.vertex_count,
                meshlet.triangle_count,
                bounds.cone_axis_s8,
                bounds.cone_cutoff_s8,
                bounds.cone_apex,
                center,
                extents,
                center,
                -1.0f,
                center,
                PK_HALF_MAX_MINUS1
            );

            output.indices.resize(output.indices.size() + meshlet.triangle_count * 3);
            memcpy(output.indices.data() + indicesOffset, meshlet_triangles.data() + meshlet.triangle_offset, meshlet.triangle_count * 3u);

            for (auto j = 0u; j < meshlet.vertex_count; ++j)
            {
                auto vertexIndex = meshlet_vertices[meshlet.vertex_offset + j];

                PKAssets::PKMeshletVertex vertex = PKAssets::PackPKMeshletVertex
                (
                    ctx->pPositions + vertexIndex * ctx->stridePositionsf32,
                    ctx->pTexcoords + vertexIndex * ctx->strideTexcoordsf32,
                    ctx->pNormals + vertexIndex * ctx->strideNormalsf32,
                    ctx->pTangents + vertexIndex * ctx->strideTangentsf32,
                    output.submesh.bbmin,
                    output.submesh.bbmax
                );

                output.vertices.push_back(vertex);
            }

            output.meshlets.push_back(pkmeshlet);
        }

        // Align triangles array size to 4bytes
        {
            auto alignedIndicesSize = 12u * ((output.indices.size() + 11ull) / 12u);

            if (output.indices.size() < alignedIndicesSize)
            {
                output.indices.resize(alignedIndicesSize);
                auto padding = alignedIndicesSize - output.indices.size();
                memset(output.indices.data() + output.indices.size(), 0u, padding);
            }
        }

        return output;
    }


    MeshStaticAssetRef CreateMeshStaticAsset(MeshStaticCollection* baseMesh, GeometryContext* ctx, const char* name)
    {
        CalculateTangents(ctx);

        auto meshlets = BuildMeshletsMonotone(ctx);

        MeshStaticAllocationData::SubMesh submesh = { 0u, ctx->countVertex, 0u, ctx->countIndex, ctx->aabb };

        MeshStaticAllocationData alloc{};
        alloc.name = name;
        // @TODO Implicit first element. not good.
        alloc.regular.pVertices = ctx->pNormals;
        alloc.regular.pIndices = ctx->pIndices;
        alloc.regular.pSubmeshes = &submesh;
        alloc.regular.indexType = ElementType::Uint;
        alloc.regular.vertexCount = ctx->countVertex;
        alloc.regular.indexCount = ctx->countIndex;
        alloc.regular.submeshCount = 1u;
        alloc.regular.streamLayout =
        {
            { ElementType::Float3, PK_RHI_VS_NORMAL, 0 },
            { ElementType::Float4, PK_RHI_VS_TANGENT, 0 },
            { ElementType::Float2, PK_RHI_VS_TEXCOORD0, 0 },
            { ElementType::Float3, PK_RHI_VS_POSITION, 1 },
        };
        alloc.meshlet.pSubmeshes = &meshlets.submesh;
        alloc.meshlet.submeshCount = 1u;
        alloc.meshlet.pMeshlets = meshlets.meshlets.data();
        alloc.meshlet.meshletCount = (uint32_t)meshlets.meshlets.size();
        alloc.meshlet.pVertices = meshlets.vertices.data();
        alloc.meshlet.vertexCount = (uint32_t)meshlets.vertices.size();
        alloc.meshlet.pIndices = meshlets.indices.data();
        alloc.meshlet.triangleCount = (uint32_t)(meshlets.indices.size() / 3ull);

        return CreateRef<MeshStaticAsset>(baseMesh, &alloc);
    }

    MeshStaticAssetRef CreateBoxMeshStaticAsset(MeshStaticCollection* baseMesh, const float3& offset, const float3& extents)
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
        } vertexData;

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
        return CreateMeshStaticAsset(baseMesh, &geometryContext, "Primitive_Box");
    }

    MeshStaticAssetRef CreateQuadMeshStaticAsset(MeshStaticCollection* baseMesh, const float2& min, const float2& max)
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
        return CreateMeshStaticAsset(baseMesh, &geometryContext, "Primitive_Box");
    }

    MeshStaticAssetRef CreatePlaneMeshStaticAsset(MeshStaticCollection* baseMesh, const float2& center, const float2& extents, uint2 resolution)
    {
        auto vcount = resolution.x * resolution.y * 4;
        auto icount = resolution.x * resolution.y * 6;
        auto* vertices = PK_CONTIGUOUS_ALLOC(Vertex_Full, vcount);
        auto* attributes = reinterpret_cast<Vertex_NormalTangentTexCoord*>(vertices);
        auto* positions = reinterpret_cast<float3*>(attributes + vcount);

        auto* indices = PK_CONTIGUOUS_ALLOC(uint, icount);
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
        auto virtualMesh = CreateMeshStaticAsset(baseMesh, &geometryContext, "Primitive_Plane"); 

        free(vertices);
        free(indices);

        return virtualMesh;
    }

    MeshStaticAssetRef CreateSphereMeshStaticAsset(MeshStaticCollection* baseMesh, const float3& offset, const float radius)
    {
        const int32_t longc = 24;
        const int32_t lattc = 16;
        const int32_t vcount = (longc + 1) * lattc + 2;

        //Vertex_Full
        auto vertices = PK_CONTIGUOUS_ALLOC(Vertex_Full, vcount);
        auto* attributes = reinterpret_cast<Vertex_NormalTangentTexCoord*>(vertices);
        auto* positions = reinterpret_cast<float3*>(attributes + vcount);

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

        const int facec = vcount;
        const int triscount = facec * 2;
        const int icount = triscount * 3;
        auto indices = PK_CONTIGUOUS_ALLOC(uint, icount);

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
        auto virtualMesh = CreateMeshStaticAsset(baseMesh, &geometryContext, "Primitive_Sphere");

        free(vertices);
        free(indices);

        return virtualMesh;
    }
}
