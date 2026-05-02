#include "PrecompiledHeader.h"
#include "Projection.h"
#include "Extended.h"

namespace PK::math
{
    void composeShadowCascadeMatrices(const ShadowCascadeCreateInfo info, float4x4* outMatrices)
    {
        typedef AABB<float,3> AABBf32;

        auto matrix = info.worldToLocal * info.clipToWorld;
        auto minNear = FLT_MAX;
        auto maxFar = -FLT_MAX;
        auto zrange = info.splitPlanes[info.count] - info.splitPlanes[0];

        AABBf32* aabbs = PK_STACK_ALLOC(AABBf32, info.count);

        for (auto i = 0u; i < info.count; ++i)
        {
            auto lnear = (info.splitPlanes[i] / zrange);
            auto lfar = (info.splitPlanes[i + 1] / zrange);

            aabbs[i] = inverseFrustumToAABB(matrix, lnear, lfar);

            aabbs[i].min.x -= info.padding;
            aabbs[i].min.y -= info.padding;
            aabbs[i].max.x += info.padding;
            aabbs[i].max.y += info.padding;

            // Quantize to resolution steps.
            // Avoids crawling effect, doesn't solve it rotationally though.
            // For that we would need to use spherical bounds, but that wastes a lot of texel density.
            auto w = aabbs[i].size().x;
            auto h = aabbs[i].size().y;
            auto c = float2(aabbs[i].center().xy);
            auto offset = mod(c, float2(w / info.resolution, h / info.resolution));

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
            zfar = max(znear + 1e-4f, zfar);
            outMatrices[i] = orthographic(aabbs[i].min.x, aabbs[i].max.x, aabbs[i].min.y, aabbs[i].max.y, znear, zfar) * info.worldToLocal;
        }
    }
}
