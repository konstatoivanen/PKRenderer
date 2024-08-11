#include "PrecompiledHeader.h"
#include "FunctionsMatrix.h"
#include "FunctionsIntersect.h"

namespace PK::Math
{
    float4x4 GetMatrixTRS(const float3& position, const quaternion& rotation, const float3& scale)
    {
        float qxx(rotation.x * rotation.x);
        float qyy(rotation.y * rotation.y);
        float qzz(rotation.z * rotation.z);
        float qxz(rotation.x * rotation.z);
        float qxy(rotation.x * rotation.y);
        float qyz(rotation.y * rotation.z);
        float qwx(rotation.w * rotation.x);
        float qwy(rotation.w * rotation.y);
        float qwz(rotation.w * rotation.z);

        float4x4 m(1.0f);
        m[3].xyz = position;
        m[0].x = scale[0] * (1.0f - 2.0f * (qyy + qzz));
        m[0].y = scale[0] * (2.0f * (qxy + qwz));
        m[0].z = scale[0] * (2.0f * (qxz - qwy));

        m[1].x = scale[1] * (2.0f * (qxy - qwz));
        m[1].y = scale[1] * (1.0f - 2.0f * (qxx + qzz));
        m[1].z = scale[1] * (2.0f * (qyz + qwx));

        m[2].x = scale[2] * (2.0f * (qxz + qwy));
        m[2].y = scale[2] * (2.0f * (qyz - qwx));
        m[2].z = scale[2] * (1.0f - 2.0f * (qxx + qyy));

        return m;
    }

    float3x4 GetMatrixTRS3x4(const float3& position, const quaternion& rotation, const float3& scale)
    {
        float qxx(rotation.x * rotation.x);
        float qyy(rotation.y * rotation.y);
        float qzz(rotation.z * rotation.z);
        float qxz(rotation.x * rotation.z);
        float qxy(rotation.x * rotation.y);
        float qyz(rotation.y * rotation.z);
        float qwx(rotation.w * rotation.x);
        float qwy(rotation.w * rotation.y);
        float qwz(rotation.w * rotation.z);

        float3x4 m(0.0f);
        m[0].w = position.x;
        m[1].w = position.y;
        m[2].w = position.z;

        m[0].x = scale[0] * (1.0f - 2.0f * (qyy + qzz));
        m[1].x = scale[0] * (2.0f * (qxy + qwz));
        m[2].x = scale[0] * (2.0f * (qxz - qwy));

        m[0].y = scale[1] * (2.0f * (qxy - qwz));
        m[1].y = scale[1] * (1.0f - 2.0f * (qxx + qzz));
        m[2].y = scale[1] * (2.0f * (qyz + qwx));

        m[0].z = scale[2] * (2.0f * (qxz + qwy));
        m[1].z = scale[2] * (2.0f * (qyz - qwx));
        m[2].z = scale[2] * (1.0f - 2.0f * (qxx + qyy));

        return m;
    }

    float4x4 GetMatrixTRS(const float3& position, const float3& euler, const float3& scale)
    {
        return GetMatrixTRS(position, glm::quat(euler), scale);
    }

    float4x4 GetMatrixInvTRS(const float3& position, const quaternion& rotation, const float3& scale)
    {
        return glm::affineInverse(GetMatrixTRS(position, rotation, scale));
    }

    float4x4 GetMatrixInvTRS(const float3& position, const float3& euler, const float3& scale)
    {
        return GetMatrixInvTRS(position, glm::quat(euler), scale);
    }

    float4x4 GetMatrixTransposeAffineInverse(const float3x4& matrix)
    {
        const float4* m = &matrix[0];

        float invDeterminant = 1.0f / (
            +m[0].x * (m[1].y * m[2].z - m[1].z * m[2].y)
           - m[0].y * (m[1].x * m[2].z - m[1].z * m[2].x)
           + m[0].z * (m[1].x * m[2].y - m[1].y * m[2].x));

        float3x3 inverse;
        float3* inv = &inverse[0];
        inv[0].x = +(m[1].y * m[2].z - m[1].z * m[2].y) * invDeterminant;
        inv[1].x = -(m[0].y * m[2].z - m[0].z * m[2].y) * invDeterminant;
        inv[2].x = +(m[0].y * m[1].z - m[0].z * m[1].y) * invDeterminant;
        inv[0].y = -(m[1].x * m[2].z - m[1].z * m[2].x) * invDeterminant;
        inv[1].y = +(m[0].x * m[2].z - m[0].z * m[2].x) * invDeterminant;
        inv[2].y = -(m[0].x * m[1].z - m[0].z * m[1].x) * invDeterminant;
        inv[0].z = +(m[1].x * m[2].y - m[1].y * m[2].x) * invDeterminant;
        inv[1].z = -(m[0].x * m[2].y - m[0].y * m[2].x) * invDeterminant;
        inv[2].z = +(m[0].x * m[1].y - m[0].y * m[1].x) * invDeterminant;

        auto offs = float3(m[0].w, m[1].w, m[2].w);
        return float4x4(float4(inv[0], 0), float4(inv[1], 0), float4(inv[2], 0), float4(-(inverse * offs), 1));
    }

    float4x4 GetMatrixTR(const float3& position, const quaternion& rotation)
    {
        float qxx(rotation.x * rotation.x);
        float qyy(rotation.y * rotation.y);
        float qzz(rotation.z * rotation.z);
        float qxz(rotation.x * rotation.z);
        float qxy(rotation.x * rotation.y);
        float qyz(rotation.y * rotation.z);
        float qwx(rotation.w * rotation.x);
        float qwy(rotation.w * rotation.y);
        float qwz(rotation.w * rotation.z);

        float4x4 m(1.0f);
        m[3].xyz = position;
        m[0][0] = 1.0f - 2.0f * (qyy + qzz);
        m[0][1] = 2.0f * (qxy + qwz);
        m[0][2] = 2.0f * (qxz - qwy);

        m[1][0] = 2.0f * (qxy - qwz);
        m[1][1] = 1.0f - 2.0f * (qxx + qzz);
        m[1][2] = 2.0f * (qyz + qwx);

        m[2][0] = 2.0f * (qxz + qwy);
        m[2][1] = 2.0f * (qyz - qwx);
        m[2][2] = 1.0f - 2.0f * (qxx + qyy);

        return m;
    }

    float3x4 TransposeTo3x4(const float4x4& matrix)
    {
        float3x4 m;

        for (auto i = 0; i < 3; ++i)
        {
            m[i][0] = matrix[0][i];
            m[i][1] = matrix[1][i];
            m[i][2] = matrix[2][i];
            m[i][3] = matrix[3][i];
        }

        return m;
    }

    float4x4 TransposeTo4x4(const float3x4& matrix)
    {
        float4x4 m;

        for (auto i = 0; i < 3; ++i)
        {
            m[0][i] = matrix[i][0];
            m[1][i] = matrix[i][1];
            m[2][i] = matrix[i][2];
            m[3][i] = matrix[i][3];
        }

        return m;
    }

    uint64_t GetMatrixHash(const float3x4& matrix, float precision)
    {
        uint64_t h = 14695981039346656037ULL;

        for (auto i = 0u; i < 3u; ++i)
        {
            h ^= (uint64_t)(matrix[i][0] / precision);
            h *= 1099511628211ULL;
            h ^= (uint64_t)(matrix[i][1] / precision);
            h *= 1099511628211ULL;
            h ^= (uint64_t)(matrix[i][2] / precision);
            h *= 1099511628211ULL;
        }

        return h;
    }

    // Produces Reverse Z
    float4x4 GetPerspective(float fov, float aspect, float zNear, float zFar)
    {
        const float tanHalfFovy = tan(fov * PK_FLOAT_DEG2RAD / 2.0f);
        float4x4 proj(0.0f);
        proj[0][0] = 1.0f / (aspect * tanHalfFovy);
        proj[1][1] = 1.0f / (tanHalfFovy);
        proj[2][2] = -zNear / (zFar - zNear);
        proj[2][3] = 1.0;
        proj[3][2] = (zNear * zFar) / (zFar - zNear);
        return proj;
    }

    // Produces Reverse Z
    float4x4 GetOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        float4x4 Result(1);
        Result[0][0] = 2.0f / (right - left);
        Result[1][1] = 2.0f / (top - bottom);
        Result[2][2] = -1.0f / (zFar - zNear);
        Result[3][0] = -(right + left) / (right - left);
        Result[3][1] = -(top + bottom) / (top - bottom);
        Result[3][2] = zFar / (zFar - zNear);
        return Result;
    }

    // Produces Reverse Z
    float4x4 GetOffsetPerspective(float left, float right, float bottom, float top, float fovy, float aspect, float zNear, float zFar)
    {
        const float tanHalfFovy = tan((fovy * PK_FLOAT_DEG2RAD) / 2.0f);
        const float rcpRL = 1.0f / (right - left);
        const float rcpTB = 1.0f / (top - bottom);
        const float rcpFN = 1.0f / (zFar - zNear);

        float4x4 Result(0);
        Result[0][0] = (2.0f * rcpRL) / (aspect * tanHalfFovy);
        Result[1][1] = (2.0f * rcpTB) / tanHalfFovy;
        Result[2][0] = -(right + left) * rcpRL;
        Result[2][1] = -(top + bottom) * rcpTB;
        Result[2][2] = -zNear * rcpFN;
        Result[2][3] = 1.0f;
        Result[3][2] = zFar * zNear * rcpFN;

        return Result;
    }

    float4x4 GetPerspectiveSubdivision(uint32_t index, const int3& gridSize, float fovy, float aspect, float znear, float zfar)
    {
        int3 coord;
        coord.x = index % gridSize.x;
        coord.y = (index / gridSize.x) % gridSize.y;
        coord.z = index / (gridSize.x * gridSize.y);

        float ix = 2.0f / gridSize.x;
        float iy = 2.0f / gridSize.y;
        float iz = 1.0f / gridSize.z;

        float x = -1.0f + (2.0f * (float)coord.x) / gridSize.x;
        float y = -1.0f + (2.0f * (float)coord.y) / gridSize.y;
        float z = (float)coord.z / gridSize.z;

        float zrange = zfar - znear;

        return GetOffsetPerspective(x, x + ix, y, y + iy, fovy, aspect, znear + zrange * z, znear + zrange * (z + iz));
    }

    float4x4 GetFrustumBoundingOrthoMatrix(const float4x4& worldToLocal, const float4x4& clipToView, const float3& paddingLD, const float3& paddingRU, float* outZNear, float* outZFar)
    {
        auto aabb = GetInverseFrustumBounds(worldToLocal * clipToView);

        *outZNear = (aabb.min.z + paddingLD.z);
        *outZFar = (aabb.max.z + paddingRU.z);

        return GetOrtho(aabb.min.x + paddingLD.x,
            aabb.max.x + paddingRU.x,
            aabb.min.y + paddingLD.y,
            aabb.max.y + paddingRU.y,
            aabb.min.z + paddingLD.z,
            aabb.max.z + paddingRU.z) * worldToLocal;
    }

    float4x4 GetPerspectiveJittered(const float4x4& matrix, const float2& jitter)
    {
        float4x4 returnValue = matrix;
        returnValue[2][0] += jitter.x;
        returnValue[2][1] += jitter.y;
        return returnValue;
    }

    void GetShadowCascadeMatrices(const ShadowCascadeCreateInfo info, float4x4* outMatrices)
    {
        auto matrix = info.worldToLocal * info.clipToWorld;
        auto minNear = std::numeric_limits<float>().max();
        auto maxFar = -std::numeric_limits<float>().max();
        auto zrange = info.splitPlanes[info.count] - info.splitPlanes[0];

        BoundingBox* aabbs = reinterpret_cast<BoundingBox*>(alloca(sizeof(BoundingBox) * info.count));

        for (auto i = 0u; i < info.count; ++i)
        {
            auto lnear = (info.splitPlanes[i] / zrange);
            auto lfar = (info.splitPlanes[i + 1] / zrange);

            aabbs[i] = GetInverseFrustumBounds(matrix, lnear, lfar);

            // Quantize to resolution steps.
            // Avoids crawling effect, doesn't solve it rotationally though.
            // For that we would need to use spherical bounds, but that wastes a lot of texel density.
            auto w = aabbs[i].GetWidth();
            auto h = aabbs[i].GetHeight();
            auto c = float2(aabbs[i].GetCenter().xy);
            auto offset = glm::mod(c, float2(w / info.resolution, h / info.resolution));

            aabbs[i].min.x -= offset.x;
            aabbs[i].min.y -= offset.y;
            aabbs[i].max.x -= offset.x;
            aabbs[i].max.y -= offset.y;

            if (aabbs[i].min.z < minNear)
            {
                minNear = aabbs[i].min.z;
            }

            if (aabbs[i].max.z > maxFar)
            {
                maxFar = aabbs[i].max.z;
            }
        }

        for (auto i = 0u; i < info.count; ++i)
        {
            auto znear = minNear + info.nearPlaneOffset;
            auto zfar = aabbs[i].max.z;
            // Ensure that z direction is retained in case near plane offset is beyond cascade z range (can happen when cascade has no shadow casters).
            zfar = glm::max(znear + 1e-4f, zfar);
            outMatrices[i] = GetOrtho(aabbs[i].min.x, aabbs[i].max.x, aabbs[i].min.y, aabbs[i].max.y, znear, zfar) * info.worldToLocal;
        }
    }
}