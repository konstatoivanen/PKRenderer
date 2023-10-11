#version 460
#pragma PROGRAM_COMPUTE
#define PK_WRITE_LIGHT_CLUSTERS
#include includes/LightResources.glsl
#include includes/Encoding.glsl

#WithAtomicCounter

#define GROUP_SIZE_X 2
#define GROUP_SIZE_Y 1
#define GROUP_SIZE_Z LIGHT_TILE_COUNT_Z
#define THREAD_COUNT (GROUP_SIZE_X * GROUP_SIZE_Y * GROUP_SIZE_Z)

PK_DECLARE_LOCAL_CBUFFER(pk_LightCount)
{
    uint LightCount;
};

struct SharedLight
{
    float3 position;
    float3 direction;
    float radius;
    float angle;
    uint type;
};

struct AABB
{
    float3 center;
    float3 extents;
    float  radius;
};

AABB currentCell;
shared SharedLight sharedLights[THREAD_COUNT];

bool IntersectPointLight(uint lightIndex)
{
    SharedLight light = sharedLights[lightIndex];
    float3 d = abs(light.position.xyz - currentCell.center) - currentCell.extents;
    float r = light.radius - cmax(min(d, float3(0.0f)));
    d = max(d, float3(0.0f));
    return light.radius > 0.0f && dot(d, d) <= r * r;
}

// Source: https://bartwronski.com/2017/04/13/cull-that-cone/
bool IntersectSpotLight(uint lightIndex)
{
    SharedLight light = sharedLights[lightIndex];

    float3 V = currentCell.center - light.position;
    float  VlenSq = dot(V, V);
    float  V1len = dot(V, light.direction);
    float  distanceClosestPoint = cos(light.angle * 0.5f) * sqrt(VlenSq - V1len * V1len) - V1len * sin(light.angle * 0.5f);

    const bool angleCull = distanceClosestPoint > currentCell.radius;
    const bool frontCull = V1len > currentCell.radius + light.radius;
    const bool backCull = V1len < -currentCell.radius;
    return !(angleCull || frontCull || backCull);
}

bool IntersectionTest(uint lightIndex)
{
    switch (sharedLights[lightIndex].type)
    {
        case LIGHT_TYPE_POINT: return IntersectPointLight(lightIndex);
        case LIGHT_TYPE_SPOT: return IntersectPointLight(lightIndex) && IntersectSpotLight(lightIndex);
        case LIGHT_TYPE_DIRECTIONAL: return true;
    }

    return false;
}

layout(local_size_x = GROUP_SIZE_X, local_size_y = GROUP_SIZE_Y, local_size_z = GROUP_SIZE_Z) in;
void main()
{
    const uint3 coord = gl_GlobalInvocationID;
    const uint thread = gl_LocalInvocationIndex;
    const float near = ViewDepthExp(coord.z / float(LIGHT_TILE_COUNT_Z));
    const float far = ViewDepthExp((coord.z + 1u) / float(LIGHT_TILE_COUNT_Z));

    const float4 uvminmax = saturate(float4(coord.xy, coord.xy + 1.0f) * LIGHT_TILE_SIZE_PX * pk_ScreenParams.zwzw);

    const float3 min00 = UVToViewPos(uvminmax.xy, near);
    const float3 max00 = UVToViewPos(uvminmax.xy, far);
    const float3 min11 = UVToViewPos(uvminmax.zw, near);
    const float3 max11 = UVToViewPos(uvminmax.zw, far);

    const float3 aabbmin = min(min(min00, max00), min(min11, max11));
    const float3 aabbmax = max(max(min00, max00), max(min11, max11));

    currentCell.extents = (aabbmax - aabbmin) * 0.5f;
    currentCell.center = aabbmin + currentCell.extents;
    currentCell.radius = length(currentCell.extents);

    uint visibleCount = 0;
    ushort visibleIndices[LIGHT_TILE_MAX_LIGHTS];

    const uint numBatches = (LightCount + THREAD_COUNT - 1) / THREAD_COUNT;

    for (uint batch = 0; batch < numBatches; ++batch)
    {
        const uint lightIndex = min(batch * THREAD_COUNT + thread, LightCount);
        const LightPacked packed = PK_BUFFER_DATA(pk_Lights, lightIndex);

        SharedLight light;
        light.position = WorldToViewPos(packed.LIGHT_POS);
        light.direction = WorldToViewDir(DecodeOctaUV(packed.LIGHT_PACKED_DIRECTION));
        light.radius = packed.LIGHT_RADIUS;
        light.angle = packed.LIGHT_ANGLE;
        light.type = packed.LIGHT_TYPE;
        sharedLights[thread] = light;

        barrier();

        for (uint index = 0; index < THREAD_COUNT && visibleCount < LIGHT_TILE_MAX_LIGHTS; ++index)
        {
            if (IntersectionTest(index))
            {
                visibleIndices[visibleCount++] = ushort(batch * THREAD_COUNT + index);
            }
        }
    }

    barrier();

    uint offset = 0u;

    [[branch]]
    if (All_Less(coord.xy, LIGHT_TILE_COUNT_XY))
    {
        offset = PK_AtomicCounterAdd(visibleCount);

        for (uint i = 0; i < visibleCount; ++i)
        {
            PK_BUFFER_DATA(pk_LightLists, offset + i) = visibleIndices[i];
        }
    }

    uint base = 0;
    base = bitfieldInsert(base, offset, 0, 22);
    base = bitfieldInsert(base, visibleCount, 22, 8);
    base = bitfieldInsert(base, GetShadowCascadeIndex(lerp(near, far, 0.5f)), 30, 2);
    imageStore(pk_LightTiles, int3(coord), uint4(base));
}
