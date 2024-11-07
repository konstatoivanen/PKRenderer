
#pragma pk_program SHADER_STAGE_COMPUTE main
#define PK_USE_SINGLE_DESCRIPTOR_SET
#include "includes/Common.glsl"
#include "includes/BRDF.glsl"
#include "includes/Noise.glsl"

layout(rgba16f, set = PK_SET_DRAW) uniform writeonly restrict image2D pk_Image;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
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

    float A = 0;
    float B = 0;
    float C = 0;

    for (uint i = 0; i < sample_count; ++i)
    {
        float2 Xi = Hammersley(i, sample_count);

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
                    A += NoL_Vis_PDF * Fc;
                    B += NoL_Vis_PDF;
                #else
                    float Fc = pow(1 - VoH, 5);
                    A += NoL_Vis_PDF * (1 - Fc);
                    B += NoL_Vis_PDF * Fc;
                #endif
            }
        }

        // Diffuse term
        {
            float Phi = PK_TWO_PI * Xi.x;
            float CosTheta = sqrt(Xi.y);
            float SinTheta = sqrt(1 - CosTheta * CosTheta);

            float3 L;
            L.x = SinTheta * cos(Phi);
            L.y = SinTheta * sin(Phi);
            L.z = CosTheta;
            float3 H = normalize(V + L);

            float LoH = saturate(dot(L, H));
            float NoL = saturate(L.z);
            float NoH = saturate(H.z);
            float VoH = saturate(dot(V, H));
            
            if (NoL > 0)
            {
                C += Fd_Chan(NoV, NoL, NoH, LoH, 1.0f, pow2(roughness));
            }
        }
    }

    imageStore(pk_Image, coord, float4(float3(A, B, C) / sample_count, 0.0f));
}
