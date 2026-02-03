
#pragma pk_program SHADER_STAGE_COMPUTE IntegrateCs

#include "includes/Common.glsl"
#include "includes/BRDF.glsl"
#include "includes/Noise.glsl"

uniform writeonly restrict image2D pk_Image;

[pk_numthreads(8u, 8u, 1u)]
void IntegrateCs()
{
    const int size = imageSize(pk_Image).x;
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float NoV = (coord.x + 0.5f) / size;
    const float roughness = (coord.y + 0.5f) / size;
    const uint sample_count = 1024u;

    float3 V;
    V.x = sqrt(1.0f - NoV * NoV);	// sin
    V.y = 0;
    V.z = NoV;						// cos

    float4 accum = 0.0f.xxxx;

    for (uint i = 0; i < sample_count; ++i)
    {
        float2 Xi = Hammersley(i, sample_count);

        // GGX Specular term
        {
            float3 H = Fr_Inverse_GGX(Xi, pow2(roughness));
            float3 L = 2 * dot(V, H) * H - V;

            float NoL = saturate(L.z);
            float NoH = saturate(H.z);
            float VoH = saturate(dot(V, H));

            if (NoL > 0)
            {
                const float	Vis = V_SmithGGXCorrelated(NoL, NoV, pow2(roughness));

                // Incident light = NoL
                // pdf = D * NoH / (4 * VoH)
                // NoL * Vis / pdf
                float NoL_Vis_PDF = NoL * Vis * (4 * VoH / NoH);

                #if PK_DFG_USE_MULTIPLE_SCATTER
                    float Fc = pow(1 - VoH, 5);
                    accum.x += NoL_Vis_PDF * Fc;
                    accum.y += NoL_Vis_PDF;
                #else
                    float Fc = pow(1 - VoH, 5);
                    accum.x += NoL_Vis_PDF * (1 - Fc);
                    accum.y += NoL_Vis_PDF * Fc;
                #endif
            }
        }

        // Diffuse term
        {
            float phi = PK_TWO_PI * Xi.x;
            float cosTheta = sqrt(Xi.y);
            float sinTheta = sqrt(1 - cosTheta * cosTheta);

            float3 L;
            L.x = sinTheta * cos(phi);
            L.y = sinTheta * sin(phi);
            L.z = cosTheta;
            float3 H = normalize(V + L);

            float LoH = saturate(dot(L, H));
            float NoL = saturate(L.z);
            float NoH = saturate(H.z);
            float VoH = saturate(dot(V, H));
            
            if (NoL > 0)
            {
                accum.z += Fd_Chan(NoV, NoL, NoH, LoH, 1.0f, pow2(roughness));
            }
        }

        // Cloth term
        {
            float phi = PK_TWO_PI * Xi.x;
            float cosTheta = 1.0f - Xi.y;
            float sinTheta = sqrt(1 - cosTheta * cosTheta);

            float3 H;
            H.x = sinTheta * cos(phi);
            H.y = sinTheta * sin(phi);
            H.z = cosTheta;
            float3 L = 2 * dot(V, H) * H - V;

            float VoH = saturate(dot(V, H));
            float NoL = saturate(L.z);
            float NoH = saturate(H.z);

            if (NoL > 0)
            {
                const float V = V_Neubelt(NoV, NoL);
                const float D = D_Charlie(NoH, pow2(roughness));
                accum.w += V * D * NoL * VoH * (4.0f * PK_TWO_PI);
            }
        }
    }

    imageStore(pk_Image, coord, accum / sample_count);
}
