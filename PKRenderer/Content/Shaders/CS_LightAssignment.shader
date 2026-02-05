
#pragma pk_program SHADER_STAGE_COMPUTE LightAssignmentCs

#define PK_WRITE_LIGHT_CLUSTERS
#define GROUP_SIZE_X 2
#define GROUP_SIZE_Y 1
#define GROUP_SIZE_Z LIGHT_TILE_COUNT_Z
#define THREAD_COUNT (GROUP_SIZE_X * GROUP_SIZE_Y * GROUP_SIZE_Z)

#include "includes/LightResources.glsl"
#include "includes/Encoding.glsl"

PK_DECLARE_LOCAL_CBUFFER(pk_LastLightIndex)
{
    uint LastLightIndex;
};

uniform RWBuffer<uint, 1u> pk_LightCounter;

struct SharedLight
{
    float3 position;
    float radius;
    float3 direction;
    float cosAngle;
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

    const float3 to_cell = current_cell.center - light.position;
    const float dist_sqr = dot(to_cell, to_cell);
    const float dist_proj = dot(to_cell, light.direction);
    const float dist_cone = light.cosAngle * sqrt(dist_sqr - pow2(dist_proj)) - dist_proj * sqrt(1.0f - pow2(light.cosAngle));

    const bool cull_angle = dist_cone > current_cell.radius;
    const bool cull_front = dist_proj > current_cell.radius + light.radius;
    const bool cull_back = dist_proj < -current_cell.radius;
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

[pk_numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void LightAssignmentCs()
{
    const uint3 coord = gl_GlobalInvocationID;
    const uint thread = gl_LocalInvocationIndex;
    const float near = ViewDepthExp(coord.z, pk_LightTileZParams.xyz);
    const float far = ViewDepthExp(coord.z + 1u, pk_LightTileZParams.xyz);

    const float4 uvminmax = saturate(float4(coord.xy, coord.xy + 1.0f) * LIGHT_TILE_SIZE_PX * pk_ScreenParams.zwzw);

    const float3 min00 = UvToViewPos(uvminmax.xy, near);
    const float3 max00 = UvToViewPos(uvminmax.xy, far);
    const float3 min11 = UvToViewPos(uvminmax.zw, near);
    const float3 max11 = UvToViewPos(uvminmax.zw, far);

    const float3 aabb_min = min(min(min00, max00), min(min11, max11));
    const float3 aabb_max = max(max(min00, max00), max(min11, max11));

    current_cell.extents = (aabb_max - aabb_min) * 0.5f;
    current_cell.center = aabb_min + current_cell.extents;
    current_cell.radius = length(current_cell.extents);

    uint visible_count = 0;
    ushort visible_indices[LIGHT_TILE_MAX_LIGHTS];

    const uint light_count = LastLightIndex + 1u;
    const uint batch_count = (light_count + THREAD_COUNT - 1) / THREAD_COUNT;

    for (uint batch = 0; batch < batch_count; ++batch)
    {
        const uint light_index = min(batch * THREAD_COUNT + thread, LastLightIndex);
        const SceneLight scene_light = Lights_LoadLight(light_index);

        SharedLight light;
        light.position = WorldToViewPos(scene_light.position);
        light.direction = WorldToViewVec(Quat_MultiplyVector(scene_light.rotation, float3(0,0,1)));
        light.radius = scene_light.radius;
        light.cosAngle = scene_light.spot_angles.x;
        light.type = scene_light.light_type;
        lds_Lights[thread] = light;

        barrier();

        for (uint index = 0; index < THREAD_COUNT; ++index)
        {
            const ushort buffer_index = ushort(batch * THREAD_COUNT + index);
            const bool is_visible = IntersectionTest(index);

            if (is_visible && buffer_index <= LastLightIndex && visible_count < LIGHT_TILE_MAX_LIGHTS)
            {
                visible_indices[visible_count++] = buffer_index;
            }
        }

        barrier();
    }

    uint offset = 0u;

    [[branch]]
    if (All_Less(coord.xy, LIGHT_TILE_COUNT_XY))
    {
        offset = atomicAdd(pk_LightCounter, visible_count);

        for (uint i = 0; i < visible_count; ++i)
        {
            pk_LightLists[offset + i] = visible_indices[i];
        }
    }

    uint base = 0;
    base = bitfieldInsert(base, offset, 0, 22);
    base = bitfieldInsert(base, visible_count, 22, 8);
    base = bitfieldInsert(base, GetShadowCascadeIndex(lerp(near, far, 0.5f)), 30, 2);
    imageStore(pk_LightTiles, int3(coord), uint4(base));
}
