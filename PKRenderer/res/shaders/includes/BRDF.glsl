#pragma once
#include Utilities.glsl
#include SharedLights.glsl

#if !defined(PK_ACTIVE_BRDF)
    #define PK_ACTIVE_BRDF BRDF_PBS_DEFAULT_DIRECT
#endif

#if !defined(PK_ACTIVE_VXGI_BRDF)
    #define PK_ACTIVE_VXGI_BRDF BRDF_VXGI_DEFAULT
#endif

struct BRDF_DATA_CACHE
{
    float3 diffuse;
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

BRDF_DATA_CACHE brdf_cache;

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

float DisneyDiffuse(float  NdotV, float NdotL, float LdotH, float perceptualRoughness)
{
    float fd90 = 0.5 + 2 * LdotH * LdotH * perceptualRoughness;
    float lightScatter = (1 + (fd90 - 1) * pow5(1 - NdotL));
    float viewScatter = (1 + (fd90 - 1) * pow5(1 - NdotV));
    return lightScatter * viewScatter;
}

float GSF_SmithGGX(float NdotL, float NdotV, float roughness)
{
    float lambdaV = NdotL * (NdotV * (1 - roughness) + roughness);
    float lambdaL = NdotV * (NdotL * (1 - roughness) + roughness);
    return 0.5f / (lambdaV + lambdaL + 1e-5f);
}

float GSF_Ashikhmin(float NdotV, float NdotL)
{
    return 1.0f / (4.0f * (NdotL + NdotV - NdotL * NdotV));
}

float NDF_GGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;
    float d = (NdotH * a2 - NdotH) * NdotH + 1.0f;
    return PK_INV_PI * a2 / (d * d + 1e-7f);
}

float NDF_Charlie(float NdotH, float roughness) 
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


void INIT_BRDF_CACHE(float3 diffuse, 
                     float3 specular, 
                     float3 sheen, 
                     float3 normal, 
                     float3 viewdir, 
                     float reflectivity,
                     float roughness, 
                     float subsurface_distortion, 
                     float subsurface_power, 
                     float subsurface_thickness)
{
    brdf_cache.diffuse = diffuse;
    brdf_cache.specular = specular;
    brdf_cache.sheen = sheen;
    brdf_cache.normal = normal;
    brdf_cache.viewdir = viewdir;
    brdf_cache.reflectivity = reflectivity;
    brdf_cache.roughness = roughness;
    brdf_cache.perceptualRoughness = sqrt(roughness);
    brdf_cache.subsurface = float3(subsurface_distortion, subsurface_power, subsurface_thickness);
    brdf_cache.nv = max(0.0f, dot(normal, viewdir));
}

float3 BRDF_PBS_DEFAULT_INDIRECT(const Indirect gi)
{
    float surfaceReduction = 1.0 / (brdf_cache.roughness * brdf_cache.roughness + 1.0);
    float grazingTerm = saturate((1 - brdf_cache.perceptualRoughness) + brdf_cache.reflectivity);
    return brdf_cache.diffuse * gi.diffuse.rgb + surfaceReduction * gi.specular.rgb * FresnelLerp(brdf_cache.specular, grazingTerm.xxx, brdf_cache.nv);
}

float3 BRDF_PBS_DEFAULT_DIRECT(const Light light)
{
    float3 color = light.color * light.shadow;
    
    float3 halfDir = normalize(light.direction.xyz + brdf_cache.viewdir);
    float3 ldots;
    ldots.x = dot(brdf_cache.normal, light.direction.xyz); // NL
    ldots.y = dot(brdf_cache.normal, halfDir); // NH
    ldots.z = dot(light.direction.xyz, halfDir); // LH
    ldots = max(0.0f.xxx, ldots);

    float G = GSF_SmithGGX(ldots.x, brdf_cache.nv, brdf_cache.roughness);
    float D = NDF_GGX(ldots.y, brdf_cache.roughness);
    
    float diffuseTerm = DisneyDiffuse(brdf_cache.nv, ldots.x, ldots.z, brdf_cache.perceptualRoughness) * ldots.x * PK_INV_PI;
    float specularTerm = max(0.0f, G * D * ldots.x);
    
    return brdf_cache.diffuse * color * diffuseTerm + specularTerm * color * FresnelTerm(brdf_cache.specular, ldots.z);
}

float3 BRDF_PBS_DEFAULT_SS(const Light light)
{
    float3 color = light.color * light.shadow;
    
    float3 halfDir = normalize(light.direction.xyz + brdf_cache.viewdir);
    float3 ldots;
    ldots.x = dot(brdf_cache.normal, light.direction.xyz); // NL
    ldots.y = dot(brdf_cache.normal, halfDir); // NH
    ldots.z = dot(light.direction.xyz, halfDir); // LH
    ldots = max(0.0f.xxx, ldots);

    
    float S = TDF_Dice(brdf_cache.viewdir, light.direction.xyz, brdf_cache.normal, brdf_cache.subsurface);
    float G = GSF_SmithGGX(ldots.x, brdf_cache.nv, brdf_cache.roughness);
    float D = NDF_GGX(ldots.y, brdf_cache.roughness);
    
    float diffuseTerm = DisneyDiffuse(brdf_cache.nv, ldots.x, ldots.z, brdf_cache.perceptualRoughness) * ldots.x * PK_INV_PI;
    float specularTerm = max(0.0f, G * D * ldots.x);
    
    return brdf_cache.diffuse * color * diffuseTerm + 
           brdf_cache.diffuse * light.color.rgb * S +
           specularTerm * color * FresnelTerm(brdf_cache.specular, ldots.z);
}

float3 BRDF_PBS_CLOTH_DIRECT(const Light light)
{
    float3 color = light.color * light.shadow;

    float3 halfDir = normalize(light.direction.xyz + brdf_cache.viewdir);
    float3 ldots;
    ldots.x = dot(brdf_cache.normal, light.direction.xyz); // NL
    ldots.y = dot(brdf_cache.normal, halfDir); // NH
    ldots.z = dot(light.direction.xyz, halfDir); // LH
    ldots = max(0.0f.xxx, ldots);

    float S = TDF_Dice(brdf_cache.viewdir, light.direction.xyz, brdf_cache.normal, brdf_cache.subsurface);
    float G = GSF_Ashikhmin(brdf_cache.nv, ldots.x);
    float D = NDF_Charlie(ldots.y, brdf_cache.roughness);

    float diffuseTerm = DisneyDiffuse(brdf_cache.nv, ldots.x, ldots.z, brdf_cache.perceptualRoughness) * ldots.x * PK_INV_PI;
    float specularTerm = max(0.0f, G * D * ldots.x);
    
    return brdf_cache.diffuse * color * diffuseTerm + 
           brdf_cache.diffuse * light.color.rgb * S +
           specularTerm * color * brdf_cache.sheen;
}

float3 BRDF_VXGI_DEFAULT(float3 albedo, float3 normal, const Light light)
{
    return albedo * light.color * light.shadow * max(0.0f, dot(light.direction, normal));
}

// @TODO make this more configurable
float3 BRDF_VXGI_CLOTH(float3 albedo, float3 normal, const Light light)
{
    return albedo * light.color * lerp(light.shadow, 1.0f, 0.5f);
}