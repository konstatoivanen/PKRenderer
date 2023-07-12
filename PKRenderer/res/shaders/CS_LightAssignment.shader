#version 460
#pragma PROGRAM_COMPUTE
#define PK_WRITE_LIGHT_CLUSTERS
#include includes/ClusterIndexing.glsl
#include includes/SharedLights.glsl

#WithAtomicCounter

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
shared SharedLight sharedLights[LIGHT_CLUSTER_GROUP_SIZE_XYZ];

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

layout(local_size_x = LIGHT_CLUSTER_TILE_COUNT_X, local_size_y = LIGHT_CLUSTER_TILE_COUNT_Y, local_size_z = LIGHT_CLUSTER_GROUP_SIZE_Z) in;
void main()
{
    const uint numBatches = (LightCount + LIGHT_CLUSTER_GROUP_SIZE_XYZ - 1) / LIGHT_CLUSTER_GROUP_SIZE_XYZ;
    const uint depthTileIndex = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * LIGHT_CLUSTER_TILE_COUNT_X;

    const float near = ViewDepthExp(gl_GlobalInvocationID.z / float(LIGHT_CLUSTER_TILE_COUNT_Z));
    const float far = ViewDepthExp((gl_GlobalInvocationID.z + 1u) / float(LIGHT_CLUSTER_TILE_COUNT_Z));

    const float2 invstep = 1.0f / float2(LIGHT_CLUSTER_TILE_COUNT_X, LIGHT_CLUSTER_TILE_COUNT_Y);
    const float4 uvminmax = float4(gl_GlobalInvocationID.xy * invstep, (gl_GlobalInvocationID.xy + 1.0f) * invstep);

    const float3 min00 = UVToViewPos(uvminmax.xy, near);
    const float3 max00 = UVToViewPos(uvminmax.xy, far);
    const float3 min11 = UVToViewPos(uvminmax.zw, near);
    const float3 max11 = UVToViewPos(uvminmax.zw, far);

    const float3 aabbmin = min(min(min00, max00), min(min11, max11));
    const float3 aabbmax = max(max(min00, max00), max(min11, max11));

    currentCell.extents = (aabbmax - aabbmin) * 0.5f;
    currentCell.center = aabbmin + currentCell.extents;
    currentCell.radius = length(currentCell.extents);

    uint visibleLightCount = 0;
    uint visibleLightIndices[LIGHT_CLUSTER_TILE_MAX_LIGHT_COUNT];

    for (uint batch = 0; batch < numBatches; ++batch)
    {
        uint lightIndex = min(batch * LIGHT_CLUSTER_GROUP_SIZE_XYZ + gl_LocalInvocationIndex, LightCount);

        PK_Light light = PK_BUFFER_DATA(pk_Lights, lightIndex);
        float4 direction = PK_BUFFER_DATA(pk_LightDirections, light.LIGHT_PROJECTION);

        SharedLight slight;
        slight.position = mul(pk_MATRIX_V, float4(light.position.xyz, 1.0f)).xyz;
        slight.direction = mul(pk_MATRIX_V, float4(direction.xyz, 0.0f)).xyz;
        slight.radius = light.position.w;
        slight.angle = direction.w;
        slight.type = light.LIGHT_TYPE;

        sharedLights[gl_LocalInvocationIndex] = slight;
        barrier();

        for (uint index = 0; index < LIGHT_CLUSTER_GROUP_SIZE_XYZ && visibleLightCount < LIGHT_CLUSTER_TILE_MAX_LIGHT_COUNT; ++index)
        {
            if (IntersectionTest(index))
            {
                visibleLightIndices[visibleLightCount++] = batch * LIGHT_CLUSTER_GROUP_SIZE_XYZ + index;
            }
        }
    }

    barrier();

    uint offset = PK_AtomicCounterAdd(visibleLightCount);

    for (uint i = 0; i < visibleLightCount; ++i)
    {
        PK_BUFFER_DATA(pk_GlobalLightsList, offset + i) = visibleLightIndices[i];
    }

    uint base = 0;
    base = bitfieldInsert(base, offset, 0, 20);
    base = bitfieldInsert(base, visibleLightCount, 20, 8);
    base = bitfieldInsert(base, GetShadowCascadeIndex(lerp(near, far, 0.5f)), 28, 4);

    imageStore(pk_LightTiles, int3(gl_GlobalInvocationID), uint4(base));
}
