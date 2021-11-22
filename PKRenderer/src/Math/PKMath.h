#pragma once
#define GLM_FORCE_SWIZZLE 
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PK::Math
{
    typedef uint16_t ushort;
    typedef uint32_t uint;
    typedef uint64_t ulong;
    typedef signed char sbyte;

    typedef glm::vec2 float2;
    typedef glm::vec3 float3;
    typedef glm::vec4 float4;

    typedef glm::mat2x2 float2x2;
    typedef glm::mat3x3 float3x3;
    typedef glm::mat4x4 float4x4;

    typedef glm::ivec2 int2;
    typedef glm::ivec3 int3;
    typedef glm::ivec4 int4;

    typedef glm::uvec2 uint2;
    typedef glm::uvec3 uint3;
    typedef glm::uvec4 uint4;

    typedef glm::bvec2 bool2;
    typedef glm::bvec3 bool3;
    typedef glm::bvec4 bool4;

    typedef glm::lowp_i8vec4 color32;
    typedef glm::vec4 color;

    typedef glm::quat quaternion;

    constexpr float2 PK_FLOAT2_ZERO = { 0.0f, 0.0f };
    constexpr float3 PK_FLOAT3_ZERO = { 0.0f, 0.0f, 0.0f };
    constexpr float4 PK_FLOAT4_ZERO = { 0.0f, 0.0f, 0.0f, 0.0f };

    constexpr int2 PK_INT2_ZERO = { 0, 0 };
    constexpr int3 PK_INT3_ZERO = { 0, 0, 0 };
    constexpr int4 PK_INT4_ZERO = { 0, 0, 0, 0 };

    constexpr uint2 PK_UINT2_ZERO = { 0, 0 };
    constexpr uint3 PK_UINT3_ZERO = { 0, 0, 0 };
    constexpr uint4 PK_UINT4_ZERO = { 0, 0, 0, 0 };

    constexpr bool2 PK_BOOL2_ZERO = { false, false };
    constexpr bool3 PK_BOOL3_ZERO = { false, false, false };
    constexpr bool4 PK_BOOL4_ZERO = { false, false, false, false };

    constexpr float2 PK_FLOAT2_ONE = { 1.0f, 1.0f };
    constexpr float3 PK_FLOAT3_ONE = { 1.0f, 1.0f, 1.0f };
    constexpr float4 PK_FLOAT4_ONE = { 1.0f, 1.0f, 1.0f, 1.0f };

    constexpr int2 PK_INT2_ONE = { 1, 1 };
    constexpr int3 PK_INT3_ONE = { 1, 1, 1 };
    constexpr int4 PK_INT4_ONE = { 1, 1, 1, 1 };

    constexpr uint2 PK_UINT2_ONE = { 1, 1 };
    constexpr uint3 PK_UINT3_ONE = { 1, 1, 1 };
    constexpr uint4 PK_UINT4_ONE = { 1, 1, 1, 1 };

    constexpr bool2 PK_BOOL2_ONE = { true, true };
    constexpr bool3 PK_BOOL3_ONE = { true, true, true };
    constexpr bool4 PK_BOOL4_ONE = { true, true, true, true };

    constexpr float2 PK_FLOAT2_UP = { 0.0f, 1.0f };
    constexpr float2 PK_FLOAT2_DOWN = { 0.0f, -1.0f };
    constexpr float2 PK_FLOAT2_LEFT = { -1.0f, 0.0f };
    constexpr float2 PK_FLOAT2_RIGHT = { 1.0f, 0.0f };

    constexpr float3 PK_FLOAT3_LEFT = { 1.0f,  0.0f,  0.0f };
    constexpr float3 PK_FLOAT3_RIGHT = { -1.0f,  0.0f,  0.0f };
    constexpr float3 PK_FLOAT3_UP = { 0.0f,  1.0f,  0.0f };
    constexpr float3 PK_FLOAT3_DOWN = { 0.0f, -1.0f,  0.0f };
    constexpr float3 PK_FLOAT3_FORWARD = { 0.0f,  0.0f,  1.0f };
    constexpr float3 PK_FLOAT3_BACKWARD = { 0.0f,  0.0f, -1.0f };

    constexpr quaternion PK_QUATERNION_IDENTITY = { 1.0f, 0.0f, 0.0f, 0.0f };

    constexpr float4x4 PK_FLOAT4X4_IDENTITY = float4x4(1.0f);

    constexpr color PK_COLOR_WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_GRAY = { 0.5f, 0.5f, 0.5f, 1.0f };
    constexpr color PK_COLOR_BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr color PK_COLOR_CLEAR = { 0.0f, 0.0f, 0.0f, 0.0f };
    constexpr color PK_COLOR_RED = { 1.0f, 0.0f, 0.0f, 1.0f };
    constexpr color PK_COLOR_GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
    constexpr color PK_COLOR_BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_CYAN = { 0.0f, 1.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_MAGENTA = { 1.0f, 0.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_YELLOW = { 1.0f, 1.0f, 0.0f, 1.0f };

    constexpr color32 PK_COLOR32_WHITE = { 255, 255, 255, 255 };
    constexpr color32 PK_COLOR32_GRAY = { 128, 128, 128, 255 };
    constexpr color32 PK_COLOR32_BLACK = { 0,   0,   0, 255 };
    constexpr color32 PK_COLOR32_CLEAR = { 0,   0,   0,   0 };
    constexpr color32 PK_COLOR32_RED = { 255,   0,   0, 255 };
    constexpr color32 PK_COLOR32_GREEN = { 0, 255,   0, 255 };
    constexpr color32 PK_COLOR32_BLUE = { 0,   0, 255, 255 };
    constexpr color32 PK_COLOR32_CYAN = { 0, 255, 255, 255 };
    constexpr color32 PK_COLOR32_MAGENTA = { 255,   0, 255, 255 };
    constexpr color32 PK_COLOR32_YELLOW = { 255, 255,   0, 255 };

    constexpr float PK_FLOAT_PI = 3.14159274F;
    constexpr float PK_FLOAT_2PI = 2.0f * 3.14159274F;
    constexpr float PK_FLOAT_DEG2RAD = 0.0174532924F;
    constexpr float PK_FLOAT_RAD2DEG = 57.29578F;

    struct FrustumPlanes
    {
        // left, right, top, bottom, near, far
        float4 planes[6];
    };

    struct BoundingBox
    {
        float3 min;
        float3 max;

        float3 GetCenter() const { return min + (max - min) * 0.5f; }
        float3 GetExtents() const { return (max - min) * 0.5f; }

        BoundingBox() : min(PK_FLOAT3_ZERO), max(PK_FLOAT3_ZERO) {}
        BoundingBox(const float3& _min, const float3& _max) : min(_min), max(_max) {}
    };

    namespace Functions
    {
        float4x4 GetMatrixTRS(const float3& position, const quaternion& rotation, const float3& scale);
        float4x4 GetMatrixTRS(const float3& position, const float3& euler, const float3& scale);
        float4x4 GetMatrixInvTRS(const float3& position, const quaternion& rotation, const float3& scale);
        float4x4 GetMatrixInvTRS(const float3& position, const float3& euler, const float3& scale);
        float4x4 GetMatrixTR(const float3& position, const quaternion& rotation);
        float4x4 GetPerspective(float fov, float aspect, float nearClip, float farClip);
        float4x4 GetOrtho(float left, float right, float bottom, float top, float zNear, float zFar);
        float4x4 GetOrtho(float left, float right, float bottom, float top, float zNear, float zFar);
        inline float GetZNearFromProj(const float4x4& matrix) { return -matrix[3][2] / (matrix[2][2] + 1.0f); }
        inline float GetZFarFromProj(const float4x4& matrix) { return -matrix[3][2] / (matrix[2][2] - 1.0f); }
        inline float GetSizeOnScreen(float depth, float sizePerDepth, float radius) { return radius / (sizePerDepth * depth); }
        float4x4 GetOffsetPerspective(float left, float right, float bottom, float top, float fovy, float aspect, float zNear, float zFar);
        float4x4 GetPerspectiveSubdivision(int index, const int3& gridSize, float fovy, float aspect, float znear, float zfar);
        float4x4 GetFrustumBoundingOrthoMatrix(const float4x4& worldToLocal, const float4x4& inverseViewProjection, const float3& paddingLD, const float3& paddingRU, float* outZnear, float* outZFar);
        float GetShadowCascadeMatrices(const float4x4& worldToLocal, const float4x4& inverseViewProjection, const float* zPlanes, float zPadding, uint count, float4x4* matrices);

        inline color HexToRGB(uint hex) { return color((hex >> 24) & 0xFF, (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, 255.0f) / 255.0f; }
        color HueToRGB(float hue);
        void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint count);
        inline float CascadeDepth(float znear, float zfar, float linearity, float interpolant) { return linearity * (znear * powf(zfar / znear, interpolant)) + (1.0f - linearity) * (znear + (zfar - znear) * interpolant); }
        inline float Cot(float value) { return cos(value) / sin(value); }
        inline float RandomFloat() { return (float)rand() / (float)RAND_MAX; }
        inline float3 RandomFloat3() { return float3(RandomFloat(), RandomFloat(), RandomFloat()); }
        inline float RandomRangeFloat(float min, float max) { return min + (RandomFloat() * (max - min)); }
        inline float3 RandomRangeFloat3(const float3& min, const float3& max) { return float3(RandomRangeFloat(min.x, max.x), RandomRangeFloat(min.y, max.y), RandomRangeFloat(min.z, max.z)); }
        inline float3 RandomEuler() { return float3(RandomRangeFloat(-360.0f, 360.0f), RandomRangeFloat(-360.0f, 360.0f), RandomRangeFloat(-360.0f, 360.0f)); }
        size_t GetNextExponentialSize(size_t start, size_t min);
        inline uint GetMaxMipLevelPow2(uint resolution) { return glm::log2(resolution); }
        inline uint GetMaxMipLevelPow2(uint2 resolution) { return glm::log2(glm::compMin(resolution)); }
        inline uint GetMaxMipLevelPow2(uint3 resolution) { return glm::log2(glm::compMin(resolution)); }
        uint GetMaxMipLevel(uint resolution);
        inline uint GetMaxMipLevel(uint2 resolution) { return GetMaxMipLevel(glm::compMin(resolution)); }
        inline uint GetMaxMipLevel(uint3 resolution) { return GetMaxMipLevel(glm::compMin(resolution)); }
        uint ByteArrayHash(const void* data, size_t count);
        ulong MurmurHash(const void* data, size_t count, ulong seed);

        // Color grading math
        color NormalizeColor(const color& color);
        // CIE xy chromaticity to CAT02 LMS.
        // http://en.wikipedia.org/wiki/LMS_color_space#CAT02
        float3 CIExyToLMS(float x, float y);
        float4 GetWhiteBalance(float temperatureShift, float tint);
        void GenerateLiftGammaGain(const color& shadows, const color& midtones, const color& highlights, color* outLift, color* outGamma, color* outGain);
        // An analytical model of chromaticity of the standard illuminant, by Judd et al.
        // http://en.wikipedia.org/wiki/Standard_illuminant#Illuminant_series_D
        // Slightly modifed to adjust it with the D65 white point (x=0.31271, y=0.32902).
        inline float StandardIlluminantY(float x) { return 2.87f * x - 3.0f * x * x - 0.27509507f; }
        inline float Luminance(const color& color) { return glm::dot(float3(0.22f, 0.707f, 0.071f), float3(color.rgb)); }
        inline float LinearToPerceptual(const color& color) { return glm::log(glm::max(Luminance(color), 0.001f)); }
        inline float PerceptualToLinear(float value) { return glm::exp(value); }

        void NormalizePlane(float4* plane);
        void ExtractFrustrumPlanes(const float4x4 viewprojection, FrustumPlanes* frustrum, bool normalize);

        float PlaneDistanceToAABB(const float4& plane, const BoundingBox& aabb);

        inline float PlaneDistanceToPoint(const float4& plane, const float3& point) { return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w; }

        inline float3 IntesectPlanes3(const float4& p1, const float4& p2, const float4& p3)
        {
            float3 n1 = p1.xyz, n2 = p2.xyz, n3 = p3.xyz;
            return ((-p1.w * glm::cross(n2, n3)) + (-p2.w * glm::cross(n3, n1)) + (-p3.w * glm::cross(n1, n2))) / (glm::dot(n1, glm::cross(n2, n3)));
        }

        inline BoundingBox CreateBoundsMinMax(const float3& min, const float3& max) { return BoundingBox(min, max); }
        inline BoundingBox CreateBoundsCenterExtents(const float3& center, const float3& extents) { return BoundingBox(center - extents, center + extents); }

        bool IntersectPlanesAABB(const float4* planes, int planeCount, const BoundingBox& aabb);
        bool IntersectAABB(const BoundingBox& a, const BoundingBox& b);
        bool IntersectSphere(const float3& center, float radius, const BoundingBox& b);
        void BoundsEncapsulate(BoundingBox* bounds, const BoundingBox& other);
        int BoundsLongestAxis(const BoundingBox& bounds);
        int BoundsShortestAxis(const BoundingBox& bounds);
        void BoundsSplit(const BoundingBox& bounds, int axis, BoundingBox* out0, BoundingBox* out1);
        bool BoundsContains(const BoundingBox& bounds, const float3& point);
        BoundingBox BoundsTransform(const float4x4& matrix, const BoundingBox& bounds);
        BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix);
        BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix, float lznear, float lzfar);
        BoundingBox GetInverseFrustumBounds(const float4x4& worldToLocal, const float4x4& inverseMatrix);
    };
}