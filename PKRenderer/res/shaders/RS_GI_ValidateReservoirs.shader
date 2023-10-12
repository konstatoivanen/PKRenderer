#version 460
#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SceneGIVX.glsl
#include includes/SceneGIReSTIR.glsl

#multi_compile _ PK_GI_CHECKERBOARD_TRACE

bool IsScreenHit(const float3 worldpos, bool isMiss)
{
    float3 clipuvw;

    if (Test_WorldToPrevClipUVW(worldpos, clipuvw))
    {
        const float rdepth = ViewDepth(clipuvw.z);
        const float sdepth = SamplePreviousViewDepth(clipuvw.xy);
        const float3 viewdir = normalize(UVToViewPos(clipuvw.xy, 1.0f));
        const float3 viewnor = SamplePreviousViewNormal(clipuvw.xy);
        const float sviewz = max(0.0f, dot(viewdir, -viewnor)) + 0.15;
        const bool isValidSurf = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);
        const bool isValidSky = isMiss && !Test_DepthFar(sdepth);
        return isValidSky || isValidSurf;
    }

    return false;
}

struct TracePayload
{
    float targetPdf;
    float linearDistance;
};

#pragma PROGRAM_RAY_GENERATION

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

void main()
{
    const int2 raycoord = int2(gl_LaunchIDEXT.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(gl_LaunchIDEXT.xy);
    const float depth = SamplePreviousViewDepth(coord);

    if (Test_DepthFar(depth))
    {
        const float3 normal = SamplePreviousWorldNormal(coord);
        const float3 viewpos = SampleViewPosition(coord, depth - depth * 1e-2f);
        const float3 viewdir = mul(normalize(viewpos), float3x3(pk_ViewToWorldPrev));
        const float3 normalOffset = normal * (0.01f / (saturate(-dot(viewdir, normal)) + 0.01f)) * 0.05f;
        const float3 origin = mul(float4(viewpos + normalOffset, 1.0f), pk_ViewToWorldPrev).xyz;
        
        Reservoir r = ReSTIR_Load(raycoord, RESTIR_LAYER_PRE);

        const float4 direction = normalizeLength(r.position - origin);
        const float tmax = direction.w + 0.5f * depth;

        payload.linearDistance = direction.w;
        payload.targetPdf = 0.0f;
        traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, 0.0f, direction.xyz, tmax, 0);

        // Validate
        if (abs(direction.w - payload.linearDistance) > (0.01f * depth) || 
            abs(ReSTIR_GetTargetPdf(r) - payload.targetPdf) > 0.5f)
        {
            ReSTIR_Store(raycoord, RESTIR_LAYER_PRE, RESTIR_RESERVOIR_ZERO);
        }
    }
}

#pragma PROGRAM_RAY_MISS
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
    const float3 worldpos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * payload.linearDistance;
    float3 radiance = 0.0f.xxx;

    if (IsScreenHit(worldpos, true))
    {
        const float2 uv = ClipToUV(mul(pk_WorldToProjPrev, float4(worldpos, 1.0f)).xyw);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        radiance = SampleEnvironment(OctaUV(gl_WorldRayDirectionEXT), 0.0f);
    }

    payload.targetPdf = dot(PK_LUMA_BT709, radiance);
}

#pragma PROGRAM_RAY_CLOSEST_HIT
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
    const float3 worldpos = PK_GET_RAY_HIT_POINT;
    const float hitdist = PK_GET_RAY_HIT_DISTANCE;
    float3 radiance = 0.0f.xxx;

    if (IsScreenHit(worldpos, false))
    {
        const float2 uv = ClipToUV(mul(pk_WorldToProjPrev, float4(worldpos, 1.0f)).xyw);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        const float4 voxel = GI_Load_Voxel(worldpos, PK_GI_VX_CONE_SIZE * log2(1.0f + (hitdist / pk_GI_VoxelSize)));
        radiance = voxel.rgb / max(voxel.a, 1e-2f);
    }

    payload.linearDistance = hitdist;
    payload.targetPdf = dot(PK_LUMA_BT709, radiance);
}
