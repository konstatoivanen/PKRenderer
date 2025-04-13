
#pragma pk_with_atomic_counter
#pragma pk_program SHADER_STAGE_COMPUTE main

#define PK_USE_CUSTOM_DESCRIPTOR_SET_INDICES
#define PK_SET_GLOBAL 0
#define PK_SET_PASS 3
#define PK_SET_SHADER 3
#define PK_SET_DRAW 3

#define PK_WRITE_LIGHT_CLUSTERS
#define GROUP_SIZE_X 2
#define GROUP_SIZE_Y 1
#define GROUP_SIZE_Z LIGHT_TILE_COUNT_Z
#define THREAD_COUNT (GROUP_SIZE_X * GROUP_SIZE_Y * GROUP_SIZE_Z)

#include "includes/LightResources.glsl"
#include "includes/Encoding.glsl"

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

AABB current_cell;
shared SharedLight lds_Lights[THREAD_COUNT];

bool IntersectPointLight(uint lightIndex)
{
    SharedLight light = lds_Lights[lightIndex];
    float3 d = abs(light.position.xyz - current_cell.center) - current_cell.extents;
    float r = light.radius - cmax(min(d, float3(0.0f)));
    d = max(d, float3(0.0f));
    return light.radius > 0.0f && dot(d, d) <= r * r;
}

// Source: https://bartwronski.com/2017/04/13/cull-that-cone/
bool IntersectSpotLight(uint lightIndex)
{
    SharedLight light = lds_Lights[lightIndex];

    float3 V = current_cell.center - light.position;
    float  VlenSq = dot(V, V);
    float  V1len = dot(V, light.direction);
    float  closest_dist = cos(light.angle * 0.5f) * sqrt(VlenSq - V1len * V1len) - V1len * sin(light.angle * 0.5f);

    const bool cull_angle = closest_dist > current_cell.radius;
    const bool cull_front = V1len > current_cell.radius + light.radius;
    const bool cull_back = V1len < -current_cell.radius;
    return !(cull_angle || cull_front || cull_back);
}

bool IntersectionTest(uint lightIndex)
{
    switch (lds_Lights[lightIndex].type)
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

    const float3 aabb_min = min(min(min00, max00), min(min11, max11));
    const float3 aabb_max = max(max(min00, max00), max(min11, max11));

    current_cell.extents = (aabb_max - aabb_min) * 0.5f;
    current_cell.center = aabb_min + current_cell.extents;
    current_cell.radius = length(current_cell.extents);

    uint visible_count = 0;
    ushort visible_indices[LIGHT_TILE_MAX_LIGHTS];

    const uint batch_count = (LightCount + THREAD_COUNT - 1) / THREAD_COUNT;

    for (uint batch = 0; batch < batch_count; ++batch)
    {
        const uint light_index = min(batch * THREAD_COUNT + thread, LightCount);
        const LightPacked packed = Lights_LoadPacked(light_index);

        SharedLight light;
        light.position = WorldToViewPos(packed.LIGHT_POS);
        light.direction = WorldToViewVec(DecodeOctaUV(packed.LIGHT_PACKED_DIRECTION));
        light.radius = packed.LIGHT_RADIUS;
        light.angle = packed.LIGHT_ANGLE;
        light.type = packed.LIGHT_TYPE;
        lds_Lights[thread] = light;

        barrier();

        for (uint index = 0; index < THREAD_COUNT && visible_count < LIGHT_TILE_MAX_LIGHTS; ++index)
        {
            if (IntersectionTest(index))
            {
                visible_indices[visible_count++] = ushort(batch * THREAD_COUNT + index);
            }
        }

        barrier();
    }

    uint offset = 0u;

    [[branch]]
    if (All_Less(coord.xy, LIGHT_TILE_COUNT_XY))
    {
        offset = PK_AtomicCounterAdd(visible_count);

        for (uint i = 0; i < visible_count; ++i)
        {
            PK_BUFFER_DATA(pk_LightLists, offset + i) = visible_indices[i];
        }
    }

    uint base = 0;
    base = bitfieldInsert(base, offset, 0, 22);
    base = bitfieldInsert(base, visible_count, 22, 8);
    base = bitfieldInsert(base, GetShadowCascadeIndex(lerp(near, far, 0.5f)), 30, 2);
    imageStore(pk_LightTiles, int3(coord), uint4(base));
}
