#pragma once
#include Utilities.glsl
#include SharedLights.glsl

#define NL x
#define NH y
#define LH z

struct BRDFSurf
{
    float3 albedo;
    float3 specular;
    float3 sheen;
    float3 normal;
    float3 viewdir;
    float3 subsurface;
    float reflectivity;
    float roughness;
    float perceptualRoughness;
    float nv;
};

BRDFSurf MakeBRDFSurf(const float3 albedo, 
                      const float3 specular, 
                      const float3 sheen, 
                      const float3 normal, 
                      const float3 viewdir, 
                      const float reflectivity,
                      const float roughness, 
                      const float subsurface_distortion, 
                      const float subsurface_power, 
                      const float subsurface_thickness)
{
    BRDFSurf surf;
    surf.albedo = albedo;
    surf.specular = specular;
    surf.sheen = sheen;
    surf.normal = normal;
    surf.viewdir = viewdir;
    surf.reflectivity = reflectivity;
    surf.roughness = roughness;
    surf.perceptualRoughness = sqrt(roughness);
    surf.subsurface = float3(subsurface_distortion, subsurface_power, subsurface_thickness);
    surf.nv = max(0.0f, dot(normal, viewdir));
    return surf;
}

float3 ComputeLightProducts(const float3 L, const float3 N, const float3 V)
{
    const float3 H = normalize(L + V);
    return max(0.0f.xxx, float3(dot(N,L), dot(N,H), dot(L,H)));
}

float3 FresnelTerm(float3 F0, float cosA)
{
    float t = pow5(1 - cosA);
    return F0 + (1-F0).xxx * t;
}

float3 FresnelLerp(float3 F0, float3 F90, float cosA)
{
    float t = pow5(1 - cosA);
    return lerp(F0, F90, t);
}

float Fd_DisneyDiffuse(float  NdotV, float NdotL, float LdotH, float perceptualRoughness)
{
    float fd90 = 0.5 + 2 * LdotH * LdotH * perceptualRoughness;
    float lightScatter = (1 + (fd90 - 1) * pow5(1 - NdotL));
    float viewScatter = (1 + (fd90 - 1) * pow5(1 - NdotV));
    return lightScatter * viewScatter;
}

float V_SmithGGX(float NdotL, float NdotV, float roughness)
{
    float lambdaV = NdotL * (NdotV * (1 - roughness) + roughness);
    float lambdaL = NdotV * (NdotL * (1 - roughness) + roughness);
    return 0.5f / (lambdaV + lambdaL + 1e-5f);
}

float V_Ashikhmin(float NdotV, float NdotL)
{
    return 1.0f / (4.0f * (NdotL + NdotV - NdotL * NdotV));
}

float D_GGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;
    float d = (NdotH * a2 - NdotH) * NdotH + 1.0f;
    return PK_INV_PI * a2 / (d * d + 1e-7f);
}

float D_Charlie(float NdotH, float roughness) 
{
    float invAlpha  = 1.0 / roughness;
    float sin2h = max(1.0 - pow2(NdotH), 0.0078125);
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) * PK_INV_TWO_PI;
}

// Source: https://www.gdcvault.com/play/1014538/Approximating-Translucency-for-a-Fast
float TDF_Dice(float3 viewdir, float3 lightdir, float3 normal, float3 subsurface)
{
    return pow(saturate(dot(viewdir, -(lightdir + normal * subsurface.x))), subsurface.y) * subsurface.z;
}

// For specular approximation in ray traced gi
float BRDF_GGX_SPECULAR_APPROX(float roughness, const float3 lightdir, const float3 viewdir, const float3 normal)
{
    const float3 P = ComputeLightProducts(lightdir, normal, viewdir);
    const float V = V_SmithGGX(P.NL, P.NH, roughness);
    const float D = D_GGX(P.LH, roughness);
    return D * V * P.NL;
}

float3 BRDF_PBS_DEFAULT_INDIRECT(const BRDFSurf surf, const Indirect gi)
{
    float surfaceReduction = 1.0 / (surf.roughness * surf.roughness + 1.0);
    float grazingTerm = saturate((1 - surf.perceptualRoughness) + surf.reflectivity);
    return surf.albedo * gi.diffuse.rgb + surfaceReduction * gi.specular.rgb * FresnelLerp(surf.specular, grazingTerm.xxx, surf.nv);
}

float3 BRDF_DEFAULT(const BRDFSurf surf, const Light light)
{
    float3 color = light.color * light.shadow;
    const float3 P = ComputeLightProducts(light.direction, surf.normal, surf.viewdir);
    float V = V_SmithGGX(P.NL, surf.nv, surf.roughness);
    float D = D_GGX(P.NH, surf.roughness);
    float Fd = Fd_DisneyDiffuse(surf.nv, P.NL, P.LH, surf.perceptualRoughness) * P.NL * PK_INV_PI;
    float Fr = max(0.0f, V * D * P.NL);
    return surf.albedo * color * Fd + 
           FresnelTerm(surf.specular, P.NH) * color * Fr;
}

float3 BRDF_SUBSURFACE(const BRDFSurf surf, const Light light)
{
    const float3 color = light.color * light.shadow;
    const float3 P = ComputeLightProducts(light.direction, surf.normal, surf.viewdir);
    const float S = TDF_Dice(surf.viewdir, light.direction, surf.normal, surf.subsurface);
    const float V = V_SmithGGX(P.NL, surf.nv, surf.roughness);
    const float D = D_GGX(P.NH, surf.roughness);
    const float Fd = Fd_DisneyDiffuse(surf.nv, P.NL, P.LH, surf.perceptualRoughness) * P.NL * PK_INV_PI;
    const float Fr = max(0.0f, V * D * P.NL);
    return surf.albedo * color * Fd + 
           surf.albedo * light.color.rgb * S +
           FresnelTerm(surf.specular, P.LH) * color * Fr * color;
}

float3 BRDF_CLOTH(const BRDFSurf surf, const Light light)
{
    const float3 color = light.color * light.shadow;
    const float3 P = ComputeLightProducts(light.direction, surf.normal, surf.viewdir);
    const float S = TDF_Dice(surf.viewdir, light.direction, surf.normal, surf.subsurface);
    const float V = V_Ashikhmin(surf.nv, P.NL);
    const float D = D_Charlie(P.NH, surf.roughness);
    const float Fd = Fd_DisneyDiffuse(surf.nv, P.NL, P.LH, surf.perceptualRoughness) * P.NL * PK_INV_PI;
    const float Fr = max(0.0f, V * D * P.NL);
    return surf.albedo * color * Fd + 
           surf.albedo * light.color.rgb * S +
           surf.sheen * color * Fr;
}

float3 BRDF_VXGI_DEFAULT(const BRDFSurf surf, const Light light)
{
    return surf.albedo * light.color * light.shadow * max(0.0f, dot(light.direction, surf.normal));
}

// @TODO make this more configurable
float3 BRDF_VXGI_CLOTH(const BRDFSurf surf, const Light light)
{
    return surf.albedo * light.color * lerp(light.shadow, 1.0f, 0.5f);
}

#undef NL
#undef NH
#undef LH