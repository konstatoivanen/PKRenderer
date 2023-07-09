
/*
required includes
includes/GBuffers.glsl
includes/Kernels.glsl
includes/BRDF.glsl

required defines

SFLT_WEIGH_ROUGHNESS // Should roughness be weighted
SFLT_NORMAL // Center normal: float xyz
SFLT_DEPTH // Center depth: float x
SFLT_ROUGHNESS // Center roughness: float x
SFLT_VIEW // Center View direction: xyz
SFLT_VPOS // Center View position: float xyz
SFLT_HISTORY // Center history: float
SFLT_STEP // Step increment: uint 
SFLT_SKIP // skip this filter: bool 
SFLT_RADIUS // Filter radius: float
SFLT_DATA_TYPE // Loaded data type
SFLT_DATA_LOAD(coord) // Data load function
SFLT_DATA_SUM(data, w) // Data sum function
SFLT_DATA_DIV(wSum) // Data div function
*/

{
    float3 disk_normal;
    const float2x3 basis = GetPrimeBasisGGX(SFLT_NORMAL, SFLT_VIEW, SFLT_ROUGHNESS, SFLT_RADIUS, disk_normal);
    const float2 rotation = make_rotation(pk_FrameIndex.y * (PK_PI / 3.0f));
    
    const float hAngle0 = GetGGXLobeHalfAngle(SFLT_ROUGHNESS, 0.985f);
    const float hAngle1 = GetGGXLobeHalfAngle(1.0f, 0.985f);
    const float hAngle2 = hAngle0 / hAngle1;

    const float k_V = 1.0f / (0.05f * SFLT_DEPTH);
    const float k_H = 1.0f / (2.0f * SFLT_RADIUS * SFLT_RADIUS);
    const float k_N = 1.0f / max(hAngle0 * lerp(0.5f, 1.0f, 1.0f / (SFLT_HISTORY + 1.0f)), 1e-4f);

    #if SFLT_WEIGH_ROUGHNESS == 1
    float2 k_R; 
    k_R.x = 1.0f / lerp(0.01f, 1.0f, SFLT_ROUGHNESS);
    k_R.y = -SFLT_ROUGHNESS * k_R.x;
    #endif

    float wSum = 1.0f;
    uint i = lerp(0u, 0xFFFFu, SFLT_SKIP);

    for (; i < 32u; i += SFLT_STEP)
    {
        const float3 s_offs = PK_POISSON_DISK_32_POW[i];
        const float2 s_uv = ViewToClipUV(SFLT_VPOS + basis * rotate2D(s_offs.xy, rotation));
        const int2 s_px = int2(s_uv * int2(pk_ScreenSize.xy));

        SFLT_DATA_TYPE s_data = SFLT_DATA_LOAD(s_px);
        const float s_depth = SampleMinZ(s_px, 0);
        const float4 s_nr = SampleViewNormalRoughness(s_px);

        const float3 s_vpos = UVToViewPos(s_uv, s_depth);
        const float3 s_ray = SFLT_VPOS - s_vpos;

        const float w_h = saturate(1.0f - abs(dot(disk_normal, s_ray)) * k_V);
        const float w_v = saturate(1.0f - dot(s_ray, s_ray) * k_H);
        const float w_n = exp(-acos(dot(SFLT_NORMAL, s_nr.xyz)) * k_N);
        
        #if SFLT_WEIGH_ROUGHNESS == 1
            const float w_r = exp(-abs(s_nr.w * k_R.x + k_R.y));
            const float w = w_h * w_v * w_n * w_r * s_offs.z;
        #else
            const float w = w_h * w_v * w_n * s_offs.z;
        #endif

        if (Test_InScreen(s_uv) && !isnan(w) && w > 1e-4f)
        {
            SFLT_DATA_SUM(s_data, w)
            wSum += w;
        }
    }

    SFLT_DATA_DIV(wSum)
}

#undef SFLT_WEIGH_ROUGHNESS 
#undef SFLT_NORMAL 
#undef SFLT_DEPTH 
#undef SFLT_ROUGHNESS 
#undef SFLT_VIEW 
#undef SFLT_VPOS 
#undef SFLT_HISTORY 
#undef SFLT_STEP 
#undef SFLT_SKIP 
#undef SFLT_RADIUS 
#undef SFLT_DATA_TYPE 
#undef SFLT_DATA_LOAD
#undef SFLT_DATA_SUM
#undef SFLT_DATA_DIV
