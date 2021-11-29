#include "PrecompiledHeader.h"
#include "PKMath.h"

namespace PK::Math
{
    float4x4 Functions::GetMatrixTRS(const float3& position, const quaternion& rotation, const float3& scale)
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
        m[0][0] = scale[0] * (1.0f - 2.0f * (qyy + qzz));
        m[0][1] = scale[0] * (2.0f * (qxy + qwz));
        m[0][2] = scale[0] * (2.0f * (qxz - qwy));

        m[1][0] = scale[1] * (2.0f * (qxy - qwz));
        m[1][1] = scale[1] * (1.0f - 2.0f * (qxx + qzz));
        m[1][2] = scale[1] * (2.0f * (qyz + qwx));

        m[2][0] = scale[2] * (2.0f * (qxz + qwy));
        m[2][1] = scale[2] * (2.0f * (qyz - qwx));
        m[2][2] = scale[2] * (1.0f - 2.0f * (qxx + qyy));

        return m;
    }

    float4x4 Functions::GetMatrixTRS(const float3& position, const float3& euler, const float3& scale)
    {
        return GetMatrixTRS(position, glm::quat(euler), scale);
    }

    float4x4 Functions::GetMatrixInvTRS(const float3& position, const quaternion& rotation, const float3& scale)
    {
        return glm::inverse(GetMatrixTRS(position, rotation, scale));
    }

    float4x4 Functions::GetMatrixInvTRS(const float3& position, const float3& euler, const float3& scale)
    {
        return GetMatrixInvTRS(position, glm::quat(euler), scale);
    }

    float4x4 Functions::GetMatrixTR(const float3& position, const quaternion& rotation)
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

    float4x4 Functions::GetPerspective(float fov, float aspect, float zNear, float zFar)
    {
        const float tanHalfFovy = tan(fov * PK_FLOAT_DEG2RAD / 2.0f);
        float4x4 proj(0.0f);
        proj[0][0] = 1.0f / (aspect * tanHalfFovy);
        proj[1][1] = 1.0f / (tanHalfFovy);
        proj[2][2] = (zFar + zNear) / (zFar - zNear);
        proj[2][3] = 1.0;
        proj[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
        return proj;
    }

    float4x4 Functions::GetOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        const float rcpRL = 1.0f / (right - left);
        const float rcpTB = 1.0f / (top - bottom);
        const float rcpFN = 1.0f / (zFar - zNear);

        float4x4 Result(1);
        Result[0][0] = -2.0f * rcpRL;
        Result[1][1] = -2.0f * rcpTB;
        Result[2][2] = 2.0f * rcpFN;
        Result[3][0] = (right + left) * rcpRL;
        Result[3][1] = (top + bottom) * rcpTB;
        Result[3][2] = -(zFar + zNear) * rcpFN;
        return Result;
    }

    float4x4 Functions::GetOffsetPerspective(float left, float right, float bottom, float top, float fovy, float aspect, float zNear, float zFar)
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
        Result[2][2] = -(zFar + zNear) * rcpFN;
        Result[2][3] = 1.0f;
        Result[3][2] = (2.0f * zFar * zNear) * rcpFN;

        return Result;
    }

    float4x4 Functions::GetPerspectiveSubdivision(int index, const int3& gridSize, float fovy, float aspect, float znear, float zfar)
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

        return Functions::GetOffsetPerspective(x, x + ix, y, y + iy, fovy, aspect, znear + zrange * z, znear + zrange * (z + iz));
    }

    float4x4 Functions::GetFrustumBoundingOrthoMatrix(const float4x4& worldToLocal, const float4x4& inverseViewProjection, const float3& paddingLD, const float3& paddingRU, float* outZNear, float* outZFar)
    {
        auto aabb = Functions::GetInverseFrustumBounds(worldToLocal * inverseViewProjection);

        *outZNear = (aabb.min.z + paddingLD.z);
        *outZFar = (aabb.max.z + paddingRU.z);

        return GetOrtho(aabb.min.x + paddingLD.x,
            aabb.max.x + paddingRU.x,
            aabb.min.y + paddingLD.y,
            aabb.max.y + paddingRU.y,
            aabb.min.z + paddingLD.z,
            aabb.max.z + paddingRU.z) * worldToLocal;
    }

    float Functions::GetShadowCascadeMatrices(const float4x4& worldToLocal, const float4x4& inverseViewProjection, const float* zPlanes, float zPadding, uint count, float4x4* matrices)
    {
        auto matrix = worldToLocal * inverseViewProjection;
        auto minNear = std::numeric_limits<float>().max();
        auto maxFar = -std::numeric_limits<float>().max();
        auto zrange = zPlanes[count] - zPlanes[0];

        BoundingBox* aabbs = reinterpret_cast<BoundingBox*>(alloca(sizeof(BoundingBox) * count));

        for (auto i = 0u; i < count; ++i)
        {
            auto lnear = zPlanes[i] / zrange;
            auto lfar = zPlanes[i + 1] / zrange;

            aabbs[i] = Functions::GetInverseFrustumBounds(matrix, lnear, lfar);

            if (aabbs[i].min.z < minNear)
            {
                minNear = aabbs[i].min.z;
            }

            if (aabbs[i].max.z > maxFar)
            {
                maxFar = aabbs[i].max.z;
            }
        }

        for (auto i = 0u; i < count; ++i)
        {
            matrices[i] = Functions::GetOrtho(
                aabbs[i].min.x,
                aabbs[i].max.x,
                aabbs[i].min.y,
                aabbs[i].max.y,
                minNear + zPadding,
                aabbs[i].max.z) * worldToLocal;
        }

        minNear += zPadding;
        return maxFar - minNear;
    }

    size_t Functions::GetNextExponentialSize(size_t start, size_t min)
    {
        if (start < 1)
        {
            start = 1;
        }

        while (start < min)
        {
            start <<= 1;
        }

        return start;
    }

    uint Functions::GetMaxMipLevel(uint resolution)
    {
        uint level = 1;

        while (resolution > 1)
        {
            ++level;
            resolution >>= 1;
        }

        return level;
    }

    uint Functions::ByteArrayHash(const void* data, size_t count)
    {
        const char* bytes = reinterpret_cast<const char*>(data);

        // Source: https://stackoverflow.com/questions/16340/how-do-i-generate-a-hashcode-from-a-byte-array-in-c
        const auto p = 16777619;
        auto hash = 2166136261;

        for (int i = 0; i < count; ++i)
        {
            hash = (hash ^ bytes[i]) * p;
        }

        hash += hash << 13;
        hash ^= hash >> 7;
        hash += hash << 3;
        hash ^= hash >> 17;
        hash += hash << 5;
        return (uint)hash;
    }

    ulong Functions::MurmurHash(const void* key, size_t len, ulong seed)
    {
        const uint32_t m = 0x5bd1e995;
        const int r = 24;

        uint32_t h1 = ((uint32_t)seed) ^ (uint32_t)len;
        uint32_t h2 = ((uint32_t)(seed >> 32));

        const uint32_t* data = (const uint32_t*)key;

        while (len >= 8)
        {
            uint32_t k1 = *data++;
            k1 *= m; k1 ^= k1 >> r; k1 *= m;
            h1 *= m; h1 ^= k1;
            len -= 4;

            uint32_t k2 = *data++;
            k2 *= m; k2 ^= k2 >> r; k2 *= m;
            h2 *= m; h2 ^= k2;
            len -= 4;
        }

        if (len >= 4)
        {
            uint32_t k1 = *data++;
            k1 *= m; k1 ^= k1 >> r; k1 *= m;
            h1 *= m; h1 ^= k1;
            len -= 4;
        }

        switch (len)
        {
            case 3: h2 ^= ((unsigned char*)data)[2] << 16;
            case 2: h2 ^= ((unsigned char*)data)[1] << 8;
            case 1: h2 ^= ((unsigned char*)data)[0];
                h2 *= m;
        };

        h1 ^= h2 >> 18; h1 *= m;
        h2 ^= h1 >> 22; h2 *= m;
        h1 ^= h2 >> 17; h1 *= m;
        h2 ^= h1 >> 19; h2 *= m;

        uint64_t h = h1;

        h = (h << 32) | h2;

        return h;
    }

    color Functions::NormalizeColor(const color& c)
    {
        auto sum = (c.r + c.g + c.b) / 3.0f;
        return sum < std::numeric_limits<float>().epsilon() ? PK_COLOR_WHITE : color(c.r / sum, c.g / sum, c.b / sum, 1.0f);
    }

    float3 Functions::CIExyToLMS(float x, float y)
    {
        auto Y = 1.0f;
        auto X = Y * x / y;
        auto Z = Y * (1.0f - x - y) / y;

        auto L = 0.7328f * X + 0.4296f * Y - 0.1624f * Z;
        auto M = -0.7036f * X + 1.6975f * Y + 0.0061f * Z;
        auto S = 0.0030f * X + 0.0136f * Y + 0.9834f * Z;

        return float3(L, M, S);
    }

    float4 Functions::GetWhiteBalance(float temperatureShift, float tint)
    {
        auto t1 = temperatureShift;
        auto t2 = tint;

        // Get the CIE xy chromaticity of the reference white point.
        // Note: 0.31271 = x value on the D65 white point
        auto x = 0.31271f - t1 * (t1 < 0.0f ? 0.1f : 0.05f);
        auto y = StandardIlluminantY(x) + t2 * 0.05f;

        // Calculate the coefficients in the LMS space.
        auto w1 = float3(0.949237f, 1.03542f, 1.08728f); // D65 white point
        auto w2 = CIExyToLMS(x, y);
        return float4(w1.x / w2.x, w1.y / w2.y, w1.z / w2.z, 1.0f);
    }

    void Functions::GenerateLiftGammaGain(const color& shadows, const color& midtones, const color& highlights, color* outLift, color* outGamma, color* outGain)
    {
        auto nLift = NormalizeColor(shadows);
        auto nGamma = NormalizeColor(midtones);
        auto nGain = NormalizeColor(highlights);

        auto avgLift = (nLift.r + nLift.g + nLift.b) / 3.0f;
        auto avgGamma = (nGamma.r + nGamma.g + nGamma.b) / 3.0f;
        auto avgGain = (nGain.r + nGain.g + nGain.b) / 3.0f;

        // Magic numbers
        const float liftScale = 0.1f;
        const float gammaScale = 0.5f;
        const float gainScale = 0.5f;

        auto liftR = (nLift.r - avgLift) * liftScale;
        auto liftG = (nLift.g - avgLift) * liftScale;
        auto liftB = (nLift.b - avgLift) * liftScale;

        auto gammaR = glm::pow(2.0f, (nGamma.r - avgGamma) * gammaScale);
        auto gammaG = glm::pow(2.0f, (nGamma.g - avgGamma) * gammaScale);
        auto gammaB = glm::pow(2.0f, (nGamma.b - avgGamma) * gammaScale);

        auto gainR = glm::pow(2.0f, (nGain.r - avgGain) * gainScale);
        auto gainG = glm::pow(2.0f, (nGain.g - avgGain) * gainScale);
        auto gainB = glm::pow(2.0f, (nGain.b - avgGain) * gainScale);

        const float minGamma = 0.01f;
        auto invGammaR = 1.0f / glm::max(minGamma, gammaR);
        auto invGammaG = 1.0f / glm::max(minGamma, gammaG);
        auto invGammaB = 1.0f / glm::max(minGamma, gammaB);

        *outLift = color(liftR, liftG, liftB, 1.0f);
        *outGamma = color(invGammaR, invGammaG, invGammaB, 1.0f);
        *outGain = color(gainR, gainG, gainB, 1.0f);
    }

    color Functions::HueToRGB(float hue)
    {
        float R = abs(hue * 6 - 3) - 1;
        float G = 2 - abs(hue * 6 - 2);
        float B = 2 - abs(hue * 6 - 4);
        return float4(glm::clamp(float3(R, G, B), PK_FLOAT3_ZERO, PK_FLOAT3_ONE), 1.0f);
    }

    void Functions::GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint count)
    {
        assert(count > 2);

        count--;
        cascades[0] = znear;
        cascades[count] = zfar;

        for (auto i = 1u; i < count; ++i)
        {
            cascades[i] = Functions::CascadeDepth(znear, zfar, linearity, i / (float)count);
        }
    }

    void Functions::NormalizePlane(float4* plane)
    {
        float mag;
        mag = sqrt(plane->x * plane->x + plane->y * plane->y + plane->z * plane->z);
        plane->x = plane->x / mag;
        plane->y = plane->y / mag;
        plane->z = plane->z / mag;
        plane->w = plane->w / mag;
    }

    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    void Functions::ExtractFrustrumPlanes(const float4x4 viewprojection, FrustumPlanes* frustrum, bool normalize)
    {
        float4* planes = frustrum->planes;
        // Left clipping plane
        planes[0].x = viewprojection[0][3] + viewprojection[0][0];
        planes[0].y = viewprojection[1][3] + viewprojection[1][0];
        planes[0].z = viewprojection[2][3] + viewprojection[2][0];
        planes[0].w = viewprojection[3][3] + viewprojection[3][0];
        // Right clipping plane
        planes[1].x = viewprojection[0][3] - viewprojection[0][0];
        planes[1].y = viewprojection[1][3] - viewprojection[1][0];
        planes[1].z = viewprojection[2][3] - viewprojection[2][0];
        planes[1].w = viewprojection[3][3] - viewprojection[3][0];
        // Top clipping plane
        planes[2].x = viewprojection[0][3] - viewprojection[0][1];
        planes[2].y = viewprojection[1][3] - viewprojection[1][1];
        planes[2].z = viewprojection[2][3] - viewprojection[2][1];
        planes[2].w = viewprojection[3][3] - viewprojection[3][1];
        // Bottom clipping plane
        planes[3].x = viewprojection[0][3] + viewprojection[0][1];
        planes[3].y = viewprojection[1][3] + viewprojection[1][1];
        planes[3].z = viewprojection[2][3] + viewprojection[2][1];
        planes[3].w = viewprojection[3][3] + viewprojection[3][1];
        // Near clipping plane
        planes[4].x = viewprojection[0][3] + viewprojection[0][2];
        planes[4].y = viewprojection[1][3] + viewprojection[1][2];
        planes[4].z = viewprojection[2][3] + viewprojection[2][2];
        planes[4].w = viewprojection[3][3] + viewprojection[3][2];
        // Far clipping plane
        planes[5].x = viewprojection[0][3] - viewprojection[0][2];
        planes[5].y = viewprojection[1][3] - viewprojection[1][2];
        planes[5].z = viewprojection[2][3] - viewprojection[2][2];
        planes[5].w = viewprojection[3][3] - viewprojection[3][2];

        // Normalize the plane equations, if requested
        if (normalize == true)
        {
            NormalizePlane(&planes[0]);
            NormalizePlane(&planes[1]);
            NormalizePlane(&planes[2]);
            NormalizePlane(&planes[3]);
            NormalizePlane(&planes[4]);
            NormalizePlane(&planes[5]);
        }
    }

    float Functions::PlaneDistanceToAABB(const float4& plane, const BoundingBox& aabb)
    {
        auto bx = plane.x > 0 ? aabb.max.x : aabb.min.x;
        auto by = plane.y > 0 ? aabb.max.y : aabb.min.y;
        auto bz = plane.z > 0 ? aabb.max.z : aabb.min.z;
        return plane.x * bx + plane.y * by + plane.z * bz + plane.w;
    }

    bool Functions::IntersectPlanesAABB(const float4* planes, int planeCount, const BoundingBox& aabb)
    {
        for (auto i = 0; i < planeCount; ++i)
        {
            auto& plane = planes[i];

            auto bx = plane.x > 0 ? aabb.max.x : aabb.min.x;
            auto by = plane.y > 0 ? aabb.max.y : aabb.min.y;
            auto bz = plane.z > 0 ? aabb.max.z : aabb.min.z;

            if (plane.x * bx + plane.y * by + plane.z * bz < -plane.w)
            {
                return false;
            }
        }

        return true;
    }

    bool Functions::IntersectAABB(const BoundingBox& a, const BoundingBox& b)
    {
        auto overlap = (!(a.min[0] > b.max[0]) && !(a.max[0] < b.min[0]));
        overlap = (!(a.min[1] > b.max[1]) && !(a.max[1] < b.min[1])) && overlap;
        overlap = (!(a.min[2] > b.max[2]) && !(a.max[2] < b.min[2])) && overlap;
        return overlap;
    }

    bool Functions::IntersectSphere(const float3& center, float radius, const BoundingBox& b)
    {
        float3 d = glm::abs(center - b.GetCenter()) - b.GetExtents();
        float r = radius - glm::compMax(min(d, float3(0.0f)));
        d = max(d, float3(0.0f));
        return radius > 0.0f && dot(d, d) <= r * r;
    }

    void Functions::BoundsEncapsulate(BoundingBox* bounds, const BoundingBox& other)
    {
        for (auto i = 0; i < 3; ++i)
        {
            if (other.min[i] < bounds->min[i])
            {
                bounds->min[i] = other.min[i];
            }

            if (other.max[i] > bounds->max[i])
            {
                bounds->max[i] = other.max[i];
            }
        }
    }

    int Functions::BoundsLongestAxis(const BoundingBox& bounds)
    {
        auto ext = bounds.GetExtents();
        auto length = ext[0];
        auto axis = 0;

        for (auto i = 1; i < 3; ++i)
        {
            if (ext[i] > length)
            {
                axis = i;
                length = ext[i];
            }
        }

        return axis;
    }

    int Functions::BoundsShortestAxis(const BoundingBox& bounds)
    {
        auto ext = bounds.GetExtents();
        auto length = ext[0];
        auto axis = 0;

        for (auto i = 1; i < 3; ++i)
        {
            if (ext[i] < length)
            {
                axis = i;
                length = ext[i];
            }
        }

        return axis;
    }

    void Functions::BoundsSplit(const BoundingBox& bounds, int axis, BoundingBox* out0, BoundingBox* out1)
    {
        *out0 = bounds;
        *out1 = bounds;
        out0->max[axis] = bounds.min[axis] + (bounds.max[axis] - bounds.min[axis]) * 0.5f;
        out1->min[axis] = bounds.min[axis] + (bounds.max[axis] - bounds.min[axis]) * 0.5f;
    }

    bool Functions::BoundsContains(const BoundingBox& bounds, const float3& point)
    {
        for (auto i = 0; i < 3; ++i)
        {
            if (bounds.min[i] > point[i] || bounds.max[i] < point[i])
            {
                return false;
            }
        }

        return true;
    }

    BoundingBox Functions::BoundsTransform(const float4x4& matrix, const BoundingBox& bounds)
    {
        BoundingBox out(matrix[3].xyz, matrix[3].xyz);

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
            {
                auto a = matrix[j][i] * bounds.min[j];
                auto b = matrix[j][i] * bounds.max[j];

                if (a < b)
                {
                    out.min[i] += a;
                    out.max[i] += b;
                }
                else
                {
                    out.min[i] += b;
                    out.max[i] += a;
                }
            }

        return out;
    }

    BoundingBox Functions::GetInverseFrustumBounds(const float4x4& inverseMatrix)
    {
        float4 positions[8];
        positions[0] = inverseMatrix * float4(-1, -1, -1, 1);
        positions[1] = inverseMatrix * float4(-1, 1, -1, 1);
        positions[2] = inverseMatrix * float4(1, 1, -1, 1);
        positions[3] = inverseMatrix * float4(1, -1, -1, 1);
        positions[4] = inverseMatrix * float4(-1, -1, 1, 1);
        positions[5] = inverseMatrix * float4(-1, 1, 1, 1);
        positions[6] = inverseMatrix * float4(1, 1, 1, 1);
        positions[7] = inverseMatrix * float4(1, -1, 1, 1);
        float3 min = { std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max() };
        float3 max = { -std::numeric_limits<float>().min(), -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max() };

        for (auto i = 0; i < 8; ++i)
        {
            positions[i] /= positions[i].w;
            max = glm::max(float3(positions[i].xyz), max);
            min = glm::min(float3(positions[i].xyz), min);
        }

        return CreateBoundsMinMax(min, max);
    }

    BoundingBox Functions::GetInverseFrustumBounds(const float4x4& inverseMatrix, float lznear, float lzfar)
    {
        float4 positions[8];
        positions[0] = inverseMatrix * float4(-1, -1, -1, 1);
        positions[1] = inverseMatrix * float4(-1, 1, -1, 1);
        positions[2] = inverseMatrix * float4(1, 1, -1, 1);
        positions[3] = inverseMatrix * float4(1, -1, -1, 1);

        positions[4] = inverseMatrix * float4(-1, -1, 1, 1);
        positions[5] = inverseMatrix * float4(-1, 1, 1, 1);
        positions[6] = inverseMatrix * float4(1, 1, 1, 1);
        positions[7] = inverseMatrix * float4(1, -1, 1, 1);

        for (auto i = 0; i < 4; ++i)
        {
            positions[i] /= positions[i].w;
            positions[i + 4] /= positions[i + 4].w;

            auto pnear = glm::mix(positions[i], positions[i + 4], lznear);
            auto pfar = glm::mix(positions[i], positions[i + 4], lzfar);

            positions[i] = pnear;
            positions[i + 4] = pfar;
        }

        float3 min = { std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max() };
        float3 max = { -std::numeric_limits<float>().min(), -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max() };

        for (auto i = 0; i < 8; ++i)
        {
            max = glm::max(float3(positions[i].xyz), max);
            min = glm::min(float3(positions[i].xyz), min);
        }

        return CreateBoundsMinMax(min, max);
    }

    BoundingBox Functions::GetInverseFrustumBounds(const float4x4& worldToLocal, const float4x4& inverseMatrix)
    {
        float4 positions[8];
        positions[0] = float4(-1, -1, -1, 1);
        positions[1] = float4(-1, 1, -1, 1);
        positions[2] = float4(1, 1, -1, 1);
        positions[3] = float4(1, -1, -1, 1);
        positions[4] = float4(-1, -1, 1, 1);
        positions[5] = float4(-1, 1, 1, 1);
        positions[6] = float4(1, 1, 1, 1);
        positions[7] = float4(1, -1, 1, 1);

        float3 min = { std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max() };
        float3 max = { -std::numeric_limits<float>().min(), -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max() };

        for (auto i = 0; i < 8; ++i)
        {
            positions[i] = inverseMatrix * positions[i];
            positions[i] /= positions[i].w;
            positions[i] = worldToLocal * positions[i];
            max = glm::max(float3(positions[i].xyz), max);
            min = glm::min(float3(positions[i].xyz), min);
        }

        return CreateBoundsMinMax(min, max);
    }
}