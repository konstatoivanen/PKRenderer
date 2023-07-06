#pragma once
#include Utilities.glsl
#include Constants.glsl

#define NL x
#define NH y
#define LH z

struct BRDFSurf
{
    float3 albedo;
    float3 F0;
    float3 sheen;
    float3 normal;
    float3 viewdir;
    float3 subsurface;
    float reflectivity;
    float roughness;
    float linearRoughness;
    float nv;
};

BRDFSurf MakeBRDFSurf(const float3 albedo, 
                      const float3 F0, 
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
    surf.F0 = F0;
    surf.sheen = sheen;
    surf.normal = normal;
    surf.viewdir = viewdir;
    surf.reflectivity = reflectivity;
    surf.roughness = roughness;
    surf.linearRoughness = sqrt(roughness);
    surf.subsurface = float3(subsurface_distortion, subsurface_power, subsurface_thickness);
    surf.nv = max(0.0f, dot(normal, viewdir));
    return surf;
}

float3 ComputeLightProducts(const float3 L, const float3 N, const float3 V)
{
    const float3 H = normalize(L + V);
    return max(0.0f.xxx, float3(dot(N,L), dot(N,H), dot(L,H)));
}

// Source: https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
float AttenuateLight(float dist, float radius) 
{
    return pow2(saturate(1.0f - pow4(dist/radius))) / (pow2(dist) + 1.0f); 
}

float PF_HenyeyGreenstein(const float3 viewdir, const float3 posToLight, float phase)
{
	float gsq = pow2(phase);
	float denom = 1.0 + gsq - 2.0 * phase * dot(viewdir, posToLight);
    return PK_INV_FOUR_PI * (1.0 - gsq) * inversesqrt(pow3(denom));
}

float PF_Schlick(const float3 viewdir, const float3 posToLight, float phase)
{
    return PK_INV_FOUR_PI * (1.0 - pow2(phase)) / pow2(1.0 - phase * dot(viewdir, posToLight));
}

float3 F_Schlick(const float3 F0, float F90, float cosA)
{
	return F0 + (F90 - F0) * pow5(1.0f - cosA);
}

float3 F_SchlickLerp(const float3 F0, float F90, float cosA)
{
	return lerp(F0, F90.xxx, pow5(1 - cosA));
}

float Fd_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    const float energyBias = lerp(0.0f, 0.5f, linearRoughness);
	const float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
	const float FD90 = energyBias + 2.0f * LdotH * LdotH * linearRoughness;
	const float lightScatter = F_Schlick(1.0f.xxx, FD90, NdotL).r;
	const float viewScatter = F_Schlick(1.0f.xxx, FD90, NdotV).r;
	return lightScatter * viewScatter * energyFactor;
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float roughness)
{
	const float a2 = pow2(roughness);
	const float LambdaV = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
	const float LambdaL = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
	return	0.5f / (LambdaV + LambdaL);
}

float V_Ashikhmin(float NdotV, float NdotL)
{
    return 1.0f / (4.0f * (NdotL + NdotV - NdotL * NdotV));
}

float D_GGX(float NdotH, float roughness)
{
    const float a2 = roughness * roughness;
    const float d = (NdotH * a2 - NdotH) * NdotH + 1.0f;
    return PK_INV_PI * a2 / (d * d + 1e-7f);
}

float D_Charlie(float NdotH, float roughness) 
{
    const float invAlpha  = 1.0 / roughness;
    const float sin2h = max(1.0 - pow2(NdotH), 0.0078125);
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) * PK_INV_TWO_PI;
}

// Source: https://www.gdcvault.com/play/1014538/Approximating-Translucency-for-a-Fast
float TDF_Dice(float3 viewdir, float3 lightdir, float3 normal, float3 subsurface)
{
    return pow(saturate(dot(viewdir, -(lightdir + normal * subsurface.x))), subsurface.y) * subsurface.z;
}

// For specular approximation in ray traced gi
float BRDF_GGX_SPECULAR_APPROX(const float3 normal, const float3 viewdir, const float roughness, const float3 lightdir)
{
    const float3 P = ComputeLightProducts(lightdir, normal, viewdir);
    const float V = V_SmithGGXCorrelated(P.NL, max(0.0f, dot(-viewdir, normal)), roughness);
    const float D = D_GGX(P.LH, roughness);
    return D * V * P.NL;
}

float3 BRDF_INDIRECT_DEFAULT(const BRDFSurf surf, const float3 diffuse, const float3 specular)
{
    const float surfaceReduction = 1.0 / (surf.roughness * surf.roughness + 1.0);
    const float F90 = saturate((1 - surf.linearRoughness) + surf.reflectivity);
    return surf.albedo * diffuse + surfaceReduction * specular * F_SchlickLerp(surf.F0, F90, surf.nv);
}


float3 BRDF_DEFAULT(const BRDFSurf surf, const float3 lightdir, const float3 lightcolor, float shadow)
{
    const float3 color = lightcolor * shadow;
    const float3 P = ComputeLightProducts(lightdir, surf.normal, surf.viewdir);
    const float V = V_SmithGGXCorrelated(P.NL, surf.nv, surf.roughness);
    const float D = D_GGX(P.NH, surf.roughness);
    const float Fd = Fd_DisneyDiffuse(surf.nv, P.NL, P.LH, surf.linearRoughness) * P.NL * PK_INV_PI;
    const float Fr = max(0.0f, V * D * P.NL);
    const float F90 = saturate(50.0 * surf.F0.g);
    return surf.albedo * color * Fd + 
           F_Schlick(surf.F0, F90, P.LH) * color * Fr;
}

float3 BRDF_SUBSURFACE(const BRDFSurf surf, const float3 lightdir, const float3 lightcolor, float shadow)
{
    const float3 color = lightcolor * shadow;
    const float3 P = ComputeLightProducts(lightdir, surf.normal, surf.viewdir);
    const float S = TDF_Dice(surf.viewdir, lightdir, surf.normal, surf.subsurface);
    const float V = V_SmithGGXCorrelated(P.NL, surf.nv, surf.roughness);
    const float D = D_GGX(P.NH, surf.roughness);
    const float Fd = Fd_DisneyDiffuse(surf.nv, P.NL, P.LH, surf.linearRoughness) * P.NL * PK_INV_PI;
    const float Fr = max(0.0f, V * D * P.NL);
    return surf.albedo * color * Fd + 
           surf.albedo * lightcolor * S +
           F_Schlick(surf.F0, 1.0f, P.LH) * color * Fr * color;
}

float3 BRDF_CLOTH(const BRDFSurf surf, const float3 lightdir, const float3 lightcolor, float shadow)
{
    const float3 color = lightcolor * shadow;
    const float3 P = ComputeLightProducts(lightdir, surf.normal, surf.viewdir);
    const float S = TDF_Dice(surf.viewdir, lightdir, surf.normal, surf.subsurface);
    const float V = V_Ashikhmin(surf.nv, P.NL);
    const float D = D_Charlie(P.NH, surf.roughness);
    const float Fd = Fd_DisneyDiffuse(surf.nv, P.NL, P.LH, surf.linearRoughness) * P.NL * PK_INV_PI;
    const float Fr = max(0.0f, V * D * P.NL);
    return surf.albedo * color * Fd + 
           surf.albedo * lightcolor * S +
           surf.sheen * color * Fr;
}

float3 BSDF_VOLUMETRIC(const float3 viewdir, const float phase, const float3 lightdir, const float3 lightcolor, float shadow)
{
    return lightcolor * shadow * PF_HenyeyGreenstein(viewdir, lightdir, phase);
}

float3 BRDF_VXGI_DEFAULT(const BRDFSurf surf, const float3 lightdir, const float3 lightcolor, float shadow)
{
    return surf.albedo * lightcolor * shadow * max(0.0f, dot(lightdir, surf.normal));
}

// @TODO make this more configurable
float3 BRDF_VXGI_CLOTH(const BRDFSurf surf, const float3 lightdir, const float3 lightcolor, float shadow)
{
    return surf.albedo * lightcolor * lerp(shadow, 1.0f, 0.5f);
}

#undef NL
#undef NH
#undef LH