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
    float3 normal;
    float3 viewdir;
    float3 sheen;
    float3 subsurface;
    float3 clearCoat;
    float sheenTint;
    float clearCoatGloss;
    float reflectivity;
    float roughness;
    float linearRoughness;
    float nv;
};

BRDFSurf MakeBRDFSurf(const float3 albedo, 
                      const float3 F0, 
                      const float3 normal, 
                      const float3 viewdir, 
                      const float3 sheen, 
                      const float3 subsurface,
                      const float3 clearCoat,
                      const float sheenTint,
                      const float clearCoatGloss,
                      const float reflectivity,
                      const float roughness)
{
    BRDFSurf surf;
    surf.albedo = albedo;
    surf.F0 = F0;
    surf.normal = normal;
    surf.viewdir = viewdir;
    surf.sheen = sheen;
    surf.subsurface = subsurface;
    surf.clearCoat = clearCoat;
    surf.clearCoatGloss = clearCoatGloss;
    surf.sheenTint = sheenTint;
    surf.reflectivity = reflectivity;
    surf.roughness = roughness;
    surf.linearRoughness = sqrt(roughness);
    surf.nv = max(0.0f, dot(normal, viewdir));
    return surf;
}

float3 ComputeLightProducts(const float3 L, const float3 N, const float3 V)
{
    const float3 H = normalize(L + V);
    return max(0.0f.xxx, float3(dot(N,L), dot(N,H), dot(L,H)));
}

/*
Sphere light sin alpha:
float falloff = 1.0f / sqrDistToLight;
float sinAlphaSqr = saturate(pow2(SourceRadius) * falloff);
float nl = dot(N, toLight);

return sqrt(sinAlphaSqr);

if (nl < SinAlpha)
{
	nl = max(nl, -SinAlpha);
	nl = pow2(sinAlpha + nl) / (4.0f * sinAlpha);
}
*/

// Source: https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
float AttenuateLight(float dist, float radius) { return pow2(saturate(1.0f - pow4(dist/radius))) / (pow2(dist) + 1.0f); }

// Source: https://www.gdcvault.com/play/1014538/Approximating-Translucency-for-a-Fast
float AttenuateLight_Translucent_Dice(float3 viewdir, float3 lightdir, float3 normal, float distortion, float power)
{
    const float3 halfdir = normalize(lightdir + normal * distortion);
    const float cosA = max(0.0f, dot(-viewdir, halfdir));
    return pow(cosA, power);
}

//Source: Ray Tracing Gems, Chapter 16 "Sampling Transformations Zoo"
float3 Fd_Inverse_Lambert(const float2 Xi, const float3 normal)
{
    float a = 1.0f - 2.0f * Xi.x;
    float b = sqrt(1.0f - a * a);
    float phi = PK_TWO_PI * Xi.y;
    // prevent grazing angles 
    a *= 0.98f;
    b *= 0.98f;
    return normalize(normal + float3(b * cos(phi), b * sin(phi), a));
}

//Source: "Sampling the GGX Distribution of Visible Normals", Heitz
float3 Fr_Inverse_GGXVNDF(float2 Xi, const float3 normal, const float3 viewdir, const float roughness)
{
    // prevent grazing angles 
    Xi *= 0.98f;
   	
    const float alpha = pow2(roughness);
    const float3x3 basis = make_TBN(normal);

    const float3 Ve = transpose(basis) * -viewdir;
    const float3 Vh = normalize(float3(alpha * Ve.x, alpha * Ve.y, Ve.z));

    const float lensq = pow2(Vh.x) + pow2(Vh.y);
    const float3 T1 = lensq > 0.0f ? float3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : float3(1,0,0);
    const float3 T2 = cross(Vh, T1);

    float r = sqrt(Xi.x);    
    float phi = PK_TWO_PI * Xi.y;    
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - s) * sqrt(1.0f - pow2(t1)) + s * t2;

    const float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - pow2(t1) - pow2(t2))) * Vh;
    const float3 Ne = normalize(float3(alpha * Nh.x, alpha * Nh.y, max(0.0f, Nh.z)));

    return basis * reflect(-Ve, Ne);
}

float PF_HenyeyGreenstein(float cosA, float g)
{
    const float gsq = pow2(g);
    const float denom = 1.0f + gsq - 2.0f * g * cosA;
    return PK_INV_FOUR_PI * (1.0f - gsq) * inversesqrt(pow3(denom));
}

float PF_HenyeyGreensteinDual(float cosA, float g0, float g1, float w) { return lerp(PF_HenyeyGreenstein(cosA, g0), PF_HenyeyGreenstein(cosA, g1), w); }

float PF_Schlick(float cosA, float g) { return PK_INV_FOUR_PI * (1.0f - pow2(g)) / pow2(1.0f - g * cosA); }

float F_Schlick(float F0, float F90, float cosA) { return F0 + (F90 - F0) * pow5(1.0f - cosA); }

float3 F_Schlick(float3 F0, float F90, float cosA) { return F0 + (F90 - F0) * pow5(1.0f - cosA); }

float3 F_SchlickLerp(float3 F0, float F90, float cosA) { return lerp(F0, F90.xxx, pow5(1.0f - cosA)); }

float Fd_Disney(float NoV, float NoL, float LoH, float linearRoughness)
{
    const float energyBias = lerp(0.0f, 0.5f, linearRoughness);
    const float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
    const float FD90 = energyBias + 2.0f * pow2(LoH) * linearRoughness;
    const float lightScatter = F_Schlick(1.0f.xxx, FD90, NoL).r;
    const float viewScatter = F_Schlick(1.0f.xxx, FD90, NoV).r;
    return lightScatter * viewScatter * energyFactor;
}

//Source: https://www.activision.com/cdn/research/MaterialAdvancesInWWII.pdf
float Fd_Chan(float NoV, float NoL, float NoH, float LoH, float FbW, float roughness)
{
    const float a2 = pow2(roughness);
	const float g = saturate((1.0f / 18.0f) * log2(2.0f / a2 - 1.0f));
    const float F0 = LoH + pow5(1.0f - LoH);
    const float F1 = (1.0f - 0.75f * pow5(1.0f - NoL)) * (1.0f - 0.75f * pow5(1.0f - NoV));
    const float Fd = F0 + (F1 - F0) * saturate(2.2f * g - 0.5f);
    const float Fb = ((34.5f * g - 59.0f) * g + 24.5f) * LoH * exp2(-max(73.2f * g - 21.2f, 8.9f) * sqrt(NoH));
    return Fd + Fb * FbW;
}

float Fss_HanrahanKrueger(float NoV, float NoL, float LoH, float roughness)
{
    const float F90 = pow2(LoH) * roughness - 1.0f;
    const float F0 = pow5(1.0f - NoV) * F90 + 1.0f;
    const float F1 = pow5(1.0f - NoL) * F90 + 1.0f;
    return 1.25f * (F0 * F1 * (1.0f / max(1e-2f, NoV + NoL) - 0.5f) + 0.5f);
}

float V_SmithGGXCorrelated(float NoL, float NoV, float roughness)
{
    const float a2 = pow2(roughness);
    const float LambdaV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    const float LambdaL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    return	0.5f / (LambdaV + LambdaL);
}

float D_GGX(float NoH, float roughness)
{
    const float a2 = pow2(roughness);
    const float d = (NoH * a2 - NoH) * NoH + 1.0f;
    return PK_INV_PI * a2 / (pow2(d) + 1e-7f);
}

// For specular approximation in ray traced gi
float BRDF_GGX_SPECULAR_APPROX(const float3 normal, const float3 viewdir, const float roughness, const float3 lightdir)
{
    const float3 P = ComputeLightProducts(lightdir, normal, viewdir);
    const float V = V_SmithGGXCorrelated(P.NL, max(0.0f, dot(-viewdir, normal)), roughness);
    const float D = D_GGX(P.LH, roughness);
    return D * V * P.NL;
}

float3 BRDF_INDIRECT(const BRDFSurf surf, const float3 diffuse, const float3 specular)
{
    const float surfaceReduction = 1.0f / (surf.roughness * surf.roughness + 1.0f);
    const float F90 = saturate((1.0f - surf.linearRoughness) + surf.reflectivity);
    return surf.albedo * diffuse + surfaceReduction * specular * F_SchlickLerp(surf.F0, F90, surf.nv);
}

float3 BRDF_DIRECT(const BRDFSurf surf, const float3 direction, const float3 radiance, float shadow)
{
    const float3 P = ComputeLightProducts(direction, surf.normal, surf.viewdir);

    float3 brdf = 0.0f.xxx;

    // Direct diffuse
    {
        const float Fd = Fd_Chan(surf.nv, P.NL, P.NH, P.LH, 1.0f, surf.roughness);
        brdf += surf.albedo * Fd * P.NL * shadow; 
    }

    // Subsurface 
    #if defined(BRDF_ENABLE_SUBSURFACE)
    {
        //@TODO This shouldn't be local to the brdf evaluation...
        const float transmittance = AttenuateLight_Translucent_Dice(surf.viewdir, direction, surf.normal, 0.2f, 16.0f);
        const float3 ss = surf.subsurface * transmittance;
        const float Fd = Fss_HanrahanKrueger(surf.nv, P.NL, P.LH, surf.roughness);

        brdf *= 1.0f.xxx - ss;
        brdf += Fd * ss;
    }
    #endif

    // Sheen
    #if defined(BRDF_ENABLE_SHEEN)
    {
        const float3 Csheen = surf.albedo / (dot(float3(0.3f, 0.6f, 0.1f), surf.albedo) + 1e-2f);
        const float3 Fsheen = surf.sheen * lerp(1.0f.xxx, Csheen, surf.sheenTint);
        brdf += Fsheen * F_Schlick(0.0f, 1.0f, P.LH) * P.NL * shadow;
    }
    #endif

    // End of diffuse eval. Normalize.
    brdf *= PK_INV_PI;

    // Direct Specular
    {
        const float V = V_SmithGGXCorrelated(P.NL, surf.nv, surf.roughness);
        const float D = D_GGX(P.NH, surf.roughness);
        const float Fr = max(0.0f, V * D * P.NL);
        const float F90 = saturate(50.0f * surf.F0.g);
        brdf += F_Schlick(surf.F0, F90, P.LH) * Fr * shadow;
    }
    
    // Clear coat specular
    #if defined(BRDF_ENABLE_CLEARCOAT)
    {
        const float V = V_SmithGGXCorrelated(P.NL, surf.nv, 0.25f);
        const float D = D_GGX(P.NH, lerp(0.1f, 0.001f, surf.clearCoatGloss));
        const float Fr = max(0.0f, 0.25f * V * D * P.NL);
        brdf += surf.clearCoat * F_Schlick(0.04f, 1.0f, P.LH) * Fr * shadow;
    }
    #endif

    return brdf * radiance;
}

float3 BRDF_VXGI(const BRDFSurf surf, const float3 direction, const float3 radiance, float shadow)
{
    float Fd = max(0.0f, dot(direction, surf.normal)) * shadow;

    #if defined(BRDF_ENABLE_SUBSURFACE)
        // Silly approximation. useful with cloth
        // @TODO parameterize
        Fd = Fd * 0.5f + 0.5f;
    #endif
    
    return surf.albedo * radiance * Fd * PK_INV_PI;
}

float3 BSDF_VOLUMETRIC(const float3 viewdir, const float phase0, const float phase1, const float phaseW, const float3 lightdir, const float3 lightcolor, float shadow)
{
    const float cosA = dot(viewdir, lightdir);
    return lightcolor * PF_HenyeyGreensteinDual(cosA, phase0, phase1, phaseW) * shadow;
}

#undef NL
#undef NH
#undef LH