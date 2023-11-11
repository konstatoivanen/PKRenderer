#include "PrecompiledHeader.h"
#include "MeshletBuilder.h"

namespace PK::Rendering::MeshUtilities
{
	using namespace PK::Assets::Mesh::Meshlet;

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


    MeshletBuildData BuildMeshletsMonotone(const float* pPositions,
                                           const float* pTexcoords,
                                           const float* pNormals,
                                           const float* pTangents,
                                           const uint32_t* indices,
                                           uint32_t vertexStride,
                                           uint32_t vertexCount,
                                           uint32_t indexCount,
                                           const Math::BoundingBox& aabb)
    {
        MeshletBuildData output{};

        const size_t vertexStridef32 = vertexStride / sizeof(float);

        size_t max_meshlets = zeux::meshopt_buildMeshletsBound(indexCount, PK_MAX_VERTICES, PK_MAX_TRIANGLES);
        std::vector<zeux::meshopt_Meshlet> meshlets(max_meshlets);
        std::vector<unsigned int> meshlet_vertices(max_meshlets * PK_MAX_VERTICES);
        std::vector<unsigned char> meshlet_triangles(max_meshlets * PK_MAX_TRIANGLES * 3);

        size_t meshlet_count = zeux::meshopt_buildMeshletsScan
        (
            meshlets.data(),
            meshlet_vertices.data(),
            meshlet_triangles.data(),
            indices,
            indexCount,
            vertexCount,
            PK_MAX_VERTICES,
            PK_MAX_TRIANGLES
        );

        output.submesh.firstTriangle = 0u;
        output.submesh.firstVertex = 0u; 
        output.submesh.firstMeshlet = 0u;
        output.submesh.triangleCount = 0u;
        output.submesh.vertexCount = 0u;
        output.submesh.meshletCount = (uint32_t)meshlet_count;
        output.submesh.bbmin[0] = aabb.min[0];
        output.submesh.bbmin[1] = aabb.min[1];
        output.submesh.bbmin[2] = aabb.min[2];
        output.submesh.bbmax[0] = aabb.max[0];
        output.submesh.bbmax[1] = aabb.max[1];
        output.submesh.bbmax[2] = aabb.max[2];

        for (auto i = 0u; i < meshlet_count; ++i)
        {
            const auto& meshlet = meshlets.at(i);

            auto bounds = zeux::meshopt_computeMeshletBounds
            (
                meshlet_vertices.data() + meshlet.vertex_offset,
                meshlet_triangles.data() + meshlet.triangle_offset,
                meshlet.triangle_count,
                pPositions,
                vertexCount,
                vertexStride
            );

            auto indicesOffset = output.indices.size();
            auto triangleOffset = indicesOffset / 3ull;
            auto verticesOffset = output.vertices.size();

            output.submesh.triangleCount += meshlet.triangle_count;
            output.submesh.vertexCount += meshlet.vertex_count;

            PKMeshlet pkmeshlet = PackMeshlet
            (
                (uint32_t)verticesOffset,
                (uint32_t)triangleOffset,
                meshlet.vertex_count,
                meshlet.triangle_count,
                bounds.cone_axis,
                bounds.center,
                bounds.radius,
                bounds.cone_apex,
                bounds.cone_cutoff
            );

            output.indices.resize(output.indices.size() + meshlet.triangle_count * 3);
            memcpy(output.indices.data() + indicesOffset, meshlet_triangles.data() + meshlet.triangle_offset * 3u, meshlet.triangle_count * 3u);

            for (auto j = 0u; j < meshlet.vertex_count; ++j)
            {
                auto vertexIndex = meshlet_vertices[meshlet.vertex_offset + j];
 
                PKVertex vertex = PackVertex
                (
                    pPositions + vertexIndex * vertexStridef32,
                    pTexcoords + vertexIndex * vertexStridef32,
                    pNormals + vertexIndex * vertexStridef32,
                    pTangents + vertexIndex * vertexStridef32,
                    bounds.center,
                    bounds.radius
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
}