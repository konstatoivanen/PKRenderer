#include "PrecompiledHeader.h"
#include "Rendering/Structs/StructsCommon.h"
#include "Rendering/MeshUtilities/AttributeUtility.h"
#include "Rendering/MeshUtilities/MeshletBuilder.h"
#include "PrimitiveBuilders.h"

namespace PK::Rendering::MeshUtilities
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    Ref<Mesh> CreateBoxMesh(const float3& offset, const float3& extents)
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

        uint32_t indices[] =
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

        auto vertexBuffer = Buffer::Create(
            {
                { ElementType::Float3, PK_VS_POSITION },
                { ElementType::Float3, PK_VS_NORMAL },
                { ElementType::Float3, PK_VS_TANGENT },
                { ElementType::Float2, PK_VS_TEXCOORD0 }
            },
            24, BufferUsage::DefaultVertex, "Box.VertexBuffer");

        auto indexBuffer = Buffer::Create(ElementType::Uint, 36, BufferUsage::DefaultIndex, "Box.IndexBuffer");
        auto submesh = SubMesh{ 0u, 24u, 0u, 36u, BoundingBox::MinMax(p3, p5) };

        auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);
        cmd->UploadBufferData(vertexBuffer.get(), vertices);
        cmd->UploadBufferData(indexBuffer.get(), indices);
        return CreateRef<Mesh>(indexBuffer, &vertexBuffer, 1u, &submesh, 1u);
    }

    Ref<Mesh> CreateQuadMesh(const float2& min, const float2& max)
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

        uint32_t indices[] =
        {
            0,1,2,
            2,3,0
        };

        auto vertexBuffer = Buffer::Create({ {ElementType::Float3, PK_VS_POSITION }, { ElementType::Float2, PK_VS_TEXCOORD0 } }, 4, BufferUsage::DefaultVertex, "Quad.VertexBuffer");
        auto indexBuffer = Buffer::Create(ElementType::Uint, 6, BufferUsage::DefaultIndex, "Quad.IndexBuffer");
        auto submesh = SubMesh{ 0u, 4u, 0u, 6u, Math::BoundingBox::MinMax({min.x, min.y, 0.0f}, {max.x, max.y, 0.0f}) };
        auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);
        cmd->UploadBufferData(vertexBuffer.get(), vertices);
        cmd->UploadBufferData(indexBuffer.get(), indices);
        return CreateRef<Mesh>(indexBuffer, &vertexBuffer, 1u, &submesh, 1u);
    }

    Ref<VirtualMesh> CreatePlaneVirtualMesh(Ref<Mesh> baseMesh, Ref<MeshletMesh> baseMeshletMesh, const float2& center, const float2& extents, uint2 resolution)
    {
        auto vcount = resolution.x * resolution.y * 4;
        auto icount = resolution.x * resolution.y * 6;
        auto* vertices = PK_CONTIGUOUS_ALLOC(Structs::Vertex_Full, vcount);
        auto* indices = PK_CONTIGUOUS_ALLOC(uint, icount);
        auto isize = float3(extents.x / resolution.x, extents.y / resolution.y, 0.0f) * 2.0f;
        auto min = float3(center - extents, 0);

        for (auto x = 0u; x < resolution.x; ++x)
        for (auto y = 0u; y < resolution.y; ++y)
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
       
        SubmeshRangeData data{};
        data.pVertices = vertices;
        data.pIndices = indices;
        data.vertexLayout =
        {
            { ElementType::Float3, PK_VS_POSITION },
            { ElementType::Float3, PK_VS_NORMAL },
            { ElementType::Float4, PK_VS_TANGENT },
            { ElementType::Float2, PK_VS_TEXCOORD0 }
        };

        data.pSubmeshes = &submesh;
        data.indexType = ElementType::Uint;
        data.vertexCount = vcount;
        data.indexCount = icount;
        data.submeshCount = 1u;

        CalculateTangents(reinterpret_cast<float*>(vertices), data.vertexLayout.GetStride() / 4, 0, 3, 6, 10, indices, vcount, icount);

        auto meshlets = BuildMeshletsMonotone
        (
            reinterpret_cast<float*>(vertices),
            reinterpret_cast<float*>(vertices) + 10ull,
            reinterpret_cast<float*>(vertices) + 3ull,
            reinterpret_cast<float*>(vertices) + 6ull,
            indices,
            data.vertexLayout.GetStride(),
            vcount,
            icount,
            submesh.bounds
        );

        MeshletRangeData meshletData;
        meshletData.pSubmeshes = &meshlets.submesh;
        meshletData.submeshCount = 1u;
        meshletData.pMeshlets = meshlets.meshlets.data();
        meshletData.meshletCount = (uint32_t)meshlets.meshlets.size();
        meshletData.pVertices = meshlets.vertices.data();
        meshletData.vertexCount = (uint32_t)meshlets.vertices.size();
        meshletData.pIndices = meshlets.indices.data();
        meshletData.triangleCount = (uint32_t)(meshlets.indices.size() / 3ull);

        auto virtualMesh = CreateRef<VirtualMesh>(baseMesh, &data, baseMeshletMesh, &meshletData);

        free(vertices);
        free(indices);

        return virtualMesh;
    }

    Ref<VirtualMesh> CreateSphereVirtualMesh(Ref<Mesh> baseMesh, Ref<MeshletMesh> baseMeshletMesh, const float3& offset, const float radius)
    {
        const int32_t longc = 24;
        const int32_t lattc = 16;
        const int32_t vcount = (longc + 1) * lattc + 2;

        //Vertex_Full
        auto vertices = PK_CONTIGUOUS_ALLOC(Structs::Vertex_Full, vcount);

        vertices[0].position = PK_FLOAT3_UP * radius;

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

        for (auto lat = 0u; lat < lattc; lat++)
        {
            for (int lon = 0u; lon <= longc; lon++)
            {
                vertices[lon + lat * (longc + 1) + 1].texcoord = float2((float)lon / longc, 1.0f - (float)(lat + 1) / (lattc + 1));
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

        SubMesh submesh = { 0u, vcount, 0u, icount, BoundingBox::CenterExtents(offset, PK_FLOAT3_ONE * radius) };
        
        SubmeshRangeData data{};
        data.pVertices = vertices;
        data.pIndices = indices;
        data.vertexLayout =
        {
            { ElementType::Float3, PK_VS_POSITION },
            { ElementType::Float3, PK_VS_NORMAL },
            { ElementType::Float4, PK_VS_TANGENT },
            { ElementType::Float2, PK_VS_TEXCOORD0 }
        };

        data.pSubmeshes = &submesh;
        data.indexType = ElementType::Uint;
        data.vertexCount = vcount;
        data.indexCount = icount;
        data.submeshCount = 1u;

        CalculateTangents(reinterpret_cast<float*>(vertices), data.vertexLayout.GetStride() / 4, 0, 3, 6, 10, indices, vcount, icount);

        auto meshlets = BuildMeshletsMonotone
        (
            reinterpret_cast<float*>(vertices),
            reinterpret_cast<float*>(vertices) + 10ull,
            reinterpret_cast<float*>(vertices) + 3ull,
            reinterpret_cast<float*>(vertices) + 6ull,
            indices,
            data.vertexLayout.GetStride(),
            vcount,
            icount,
            submesh.bounds
        );

        MeshletRangeData meshletData;
        meshletData.pSubmeshes = &meshlets.submesh;
        meshletData.submeshCount = 1u;
        meshletData.pMeshlets = meshlets.meshlets.data();
        meshletData.meshletCount = (uint32_t)meshlets.meshlets.size();
        meshletData.pVertices = meshlets.vertices.data();
        meshletData.vertexCount = (uint32_t)meshlets.vertices.size();
        meshletData.pIndices = meshlets.indices.data();
        meshletData.triangleCount = (uint32_t)(meshlets.indices.size() / 3ull);

        auto virtualMesh = CreateRef<VirtualMesh>(baseMesh, &data, baseMeshletMesh, &meshletData);

        free(vertices);
        free(indices);

        return virtualMesh;
    }
}