#pragma once
#include "Utilities.glsl"
#include "Constants.glsl"

struct BxDFSurf
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
    float alpha; // actual roughness. roughness elsewhere is linear roughness
    float nv;
};

// Source: https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
float Fatten_Default(float dist, float radius) 
{ 
    return pow2(saturate(1.0f - pow4(dist/radius))) / (pow2(dist) + 1.0f); 
}

// Source: https://www.gdcvault.com/play/1014538/Approximating-Translucency-for-a-Fast
float Fatten_Translucent_Dice(float3 viewdir, float3 lightdir, float3 normal, float distortion, float power)
{
    const float3 halfdir = normalize(lightdir + normal * distortion);
    const float cosA = max(0.0f, dot(-viewdir, halfdir));
    return pow(cosA, power);
}

// Source: https://www.guerrilla-games.com/media/News/Files/DecimaSiggraph2017.pdf
void Fsal_MaximumNHLH(float sina, float NoV, float VoL, float NoL, inout float LoH, inout float NoH)
{
    [[branch]]
    if (sina <= 0.0f)
    {   
        return;
    }

    const float cosa = sqrt(1 - pow2(sina));
    const float RoL = 2 * NoL * NoV - VoL;
    
    if (RoL >= cosa)
    {
        NoH = 1;
        LoH = abs(NoV);
        return;
    }

    const float rInvLengthT = sina * inversesqrt(1 - RoL * RoL);
    float NoTr = rInvLengthT * (NoV - RoL * NoL );
    float VoTr = rInvLengthT * (2 * NoV * NoV - 1 - RoL * VoL);

    // Newton iteration
    {
        const float NxLoV = sqrt(saturate(1 - pow2(NoL) - pow2(NoV) - pow2(VoL) + 2 * NoL * NoV * VoL));
        const float NoBr = rInvLengthT * NxLoV;
        const float VoBr = rInvLengthT * NxLoV * 2 * NoV;
        const float NoLVTr = NoL * cosa + NoV + NoTr;
        const float VoLVTr = VoL * cosa + 1.0 + VoTr;
        const float p = NoBr   * VoLVTr;
        const float q = NoLVTr * VoLVTr;
        const float s = VoBr   * NoLVTr;
        const float xNum = q * (-0.5 * p + 0.25 * VoBr * NoLVTr);
        const float xDenom = p*p + s * (s - 2 * p) + NoLVTr * ((NoL * cosa + NoV) * pow2(VoLVTr) + q * (-0.5 * (VoLVTr + VoL * cosa) - 0.5));
        const float TwoX1 = 2 * xNum / ( pow2(xDenom) + pow2(xNum) );
        const float SinTheta = TwoX1 * xDenom;
        const float CosTheta = 1.0 - TwoX1 * xNum;
        NoTr = CosTheta * NoTr + SinTheta * NoBr;
        VoTr = CosTheta * VoTr + SinTheta * VoBr;
    }

    NoL = NoL * cosa + NoTr; 
    VoL = VoL * cosa + VoTr;

    float InvLenH = inversesqrt(2 + 2 * VoL);
    NoH = saturate((NoL + NoV ) * InvLenH);
    LoH = saturate(InvLenH + InvLenH * VoL);
}

// Sphere area light ggx D energy conservation factor
float Fsal_GGXFactor(float LoH, float alpha, float sina)
{
    const float a2 = pow2(alpha);
    const float a2s = 0.25f * sina * (3.0f * alpha * sina) / (LoH + 1e-2f);
    return a2 / (a2 + a2s);
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

//Source: https://www.activision.com/cdn/research/MaterialAdvancesInWWII.pdf
float Fd_Chan(float NoV, float NoL, float NoH, float LoH, float FbW, float alpha)
{
    const float a2 = pow2(alpha);
    const float g = saturate((1.0f / 18.0f) * log2(2.0f / a2 - 1.0f));
    const float F0 = LoH + pow5(1.0f - LoH);
    const float F1 = (1.0f - 0.75f * pow5(1.0f - NoL)) * (1.0f - 0.75f * pow5(1.0f - NoV));
    const float Fd = F0 + (F1 - F0) * saturate(2.2f * g - 0.5f);
    const float Fb = ((34.5f * g - 59.0f) * g + 24.5f) * LoH * exp2(-max(73.2f * g - 21.2f, 8.9f) * sqrt(NoH));
    return Fd + Fb * FbW;
}

float Fss_HanrahanKrueger(float NoV, float NoL, float LoH, float alpha)
{
    const float F90 = pow2(LoH) * alpha - 1.0f;
    const float F0 = pow5(1.0f - NoV) * F90 + 1.0f;
    const float F1 = pow5(1.0f - NoL) * F90 + 1.0f;
    return 1.25f * (F0 * F1 * (1.0f / max(1e-2f, NoV + NoL) - 0.5f) + 0.5f);
}

float V_SmithGGXCorrelated(float NoL, float NoV, float alpha)
{
    const float a2 = pow2(alpha);
    const float LambdaV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    const float LambdaL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    return 0.5f / (LambdaV + LambdaL);
}

float D_GGX(float NoH, float alpha)
{
    const float a2 = pow2(alpha);
    const float d = (NoH * a2 - NoH) * NoH + 1.0f;
    return PK_INV_PI * a2 / (pow2(d) + 1e-7f);
}

// For specular approximation in ray traced gi
// Reminders:
// - Not multiplied by NdotL as gi data is already calculated with NdotL distribution.
// - Fresnel not present as that is applied by sampling.
// - multiplied by pi inorder to compensate for lacking energy due to NdotL distribution
float EvaluateBxDF_Specular(const float3 normal, const float3 viewdir, const float roughness, const float3 direction)
{
    const float nv = max(0.0f, dot(viewdir, normal));
    const float nl = max(0.0f, dot(normal, direction));
    const float vl = dot(viewdir, direction);
    const float ih = inversesqrt(2.0f + 2.0f * vl);
    const float nh = saturate((nl + nv) * ih);

    const float alpha = pow2(roughness);
    const float V = V_SmithGGXCorrelated(nl, nv, alpha);
    const float D = D_GGX(nh, alpha);
    // Initial diff gi samples might have zero vectors. clip them
    return max(0.0f, D * V) * step(1e-4f, nl) * PK_PI;
}

// For additional sh approximations
// Reminders:
// - Not multiplied by NdotL as gi data is already calculated with NdotL distribution.
// - Fresnel not present as that is applied by sampling.
// - multiplied by pi inorder to compensate for lacking energy due to NdotL distribution
float3 EvaluateBxDF_SpecularExtra(const BxDFSurf surf, const float3 direction, const float3 radiance)
{
    const float nl = max(0.0f, dot(surf.normal, direction));
    const float vl = dot(surf.viewdir, direction);
    const float ih = inversesqrt(2.0f + 2.0f * vl);
    float nh = saturate((nl + surf.nv) * ih);
    float lh = saturate(ih + ih * vl);

    float3 brdf = 0.0f.xxx;

    // Sheen
    #if defined(BxDF_ENABLE_SHEEN)
    {
        const float3 Csheen = surf.albedo / (dot(float3(0.3f, 0.6f, 0.1f), surf.albedo) + 1e-2f);
        const float3 Fsheen = surf.sheen * lerp(1.0f.xxx, Csheen, surf.sheenTint);
        brdf += Fsheen * F_Schlick(0.0f, 1.0f, lh);
    }
    #endif

    // End of diffuse eval. Normalize.
    brdf *= PK_INV_PI;

    // Clear coat specular
    #if defined(BxDF_ENABLE_CLEARCOAT)
    {
        // As this function is intended for sh approximations 
        // the roughness for clear coat needs to be a tad bit higher
        // due to the uniform input singal.
        const float Rc = lerp(0.15f, 0.001f, surf.clearCoatGloss);
        const float V = V_SmithGGXCorrelated(nl, surf.nv, 0.25f);
        const float D = D_GGX(nh, Rc);
        const float Fr = max(0.0f, 0.25f * V * D);
        brdf += surf.clearCoat * F_Schlick(0.04f, 1.0f, lh) * Fr;
    }
    #endif

    return brdf * radiance * PK_PI;
}

float3 EvaluateBxDF_Indirect(const BxDFSurf surf, const float3 diffuse, const float3 specular)
{
    const float surfaceReduction = 1.0f / (pow2(surf.alpha) + 1.0f);
    const float F90 = saturate((1.0f - sqrt(surf.alpha)) + surf.reflectivity);
    return surf.albedo * diffuse + surfaceReduction * specular * F_Schlick(surf.F0, F90, surf.nv);
}

float3 EvaluateBxDF_Direct(const BxDFSurf surf, const float3 direction, const float3 radiance, float shadow, float sourceRadius)
{
    const float nl = max(0.0f, dot(surf.normal, direction));
    const float vl = dot(surf.viewdir, direction);
    const float ih = inversesqrt(2.0f + 2.0f * vl);
    float nh = saturate((nl + surf.nv) * ih);
    float lh = saturate(ih + ih * vl);

    // Spherical area light tilt of LH & NH
    {
        sourceRadius = saturate(sourceRadius * (1.0f - pow2(surf.alpha)));
        Fsal_MaximumNHLH(sourceRadius, surf.nv, vl, nl, /*out*/ lh, /*out*/ nh);
    }

    float3 brdf = 0.0f.xxx;

    // Direct diffuse
    {
        // Retroreflectivity fade out for spherical area lights
        const float FbW = 1.0f; //saturate(1.0f - sourceRadius / 0.2f);
        const float Fd = Fd_Chan(surf.nv, nl, nh, lh, FbW, surf.alpha);
        brdf += surf.albedo * Fd * nl * shadow; 
    }

    // Subsurface 
    #if defined(BxDF_ENABLE_SUBSURFACE)
    {
        //@TODO This shouldn't be local to the brdf evaluation...
        const float transmittance = Fatten_Translucent_Dice(surf.viewdir, direction, surf.normal, 0.2f, 16.0f);
        const float3 ss = surf.subsurface * transmittance;
        const float Fd = Fss_HanrahanKrueger(surf.nv, nl, lh, surf.alpha);
        brdf *= 1.0f.xxx - ss;
        brdf += Fd * ss;
    }
    #endif

    // Sheen
    #if defined(BxDF_ENABLE_SHEEN)
    {
        const float3 Csheen = surf.albedo / (dot(float3(0.3f, 0.6f, 0.1f), surf.albedo) + 1e-2f);
        const float3 Fsheen = surf.sheen * lerp(1.0f.xxx, Csheen, surf.sheenTint);
        brdf += Fsheen * F_Schlick(0.0f, 1.0f, lh) * nl * shadow;
    }
    #endif

    // End of diffuse eval. Normalize.
    brdf *= PK_INV_PI;

    // Direct Specular
    {
        const float V = V_SmithGGXCorrelated(nl, surf.nv, surf.alpha);
        const float D = D_GGX(nh, surf.alpha);
        const float De = Fsal_GGXFactor(lh, surf.alpha, sourceRadius);
        const float Fr = max(0.0f, V * D * De * nl);
        const float F90 = saturate(50.0f * surf.F0.g);
        brdf += F_Schlick(surf.F0, F90, lh) * Fr * shadow;
    }
    
    // Clear coat specular
    #if defined(BxDF_ENABLE_CLEARCOAT)
    {
        const float Rc = lerp(0.1f, 0.001f, surf.clearCoatGloss);
        const float V = V_SmithGGXCorrelated(nl, surf.nv, 0.25f);
        const float D = D_GGX(nh, Rc);
        const float De = Fsal_GGXFactor(lh, Rc, sourceRadius);
        const float Fr = max(0.0f, 0.25f * V * D * De * nl);
        brdf += surf.clearCoat * F_Schlick(0.04f, 1.0f, lh) * Fr * shadow;
    }
    #endif

    return brdf * radiance;
}

float3 EvaluateBxDF_DirectMinimal(const BxDFSurf surf, const float3 direction, const float3 radiance, float shadow, float angle)
{
    float Fd = max(0.0f, dot(direction, surf.normal)) * shadow;

    #if defined(BxDF_ENABLE_SUBSURFACE)
        // Silly approximation. useful with cloth
        // @TODO parameterize
        Fd = Fd * 0.5f + 0.5f;
    #endif
    
    return surf.albedo * radiance * Fd * PK_INV_PI;
}

float3 EvaluateBxDF_Volumetric(const float3 viewdir, 
                               const float phase0, 
                               const float phase1, 
                               const float phaseW, 
                               const float3 lightdir, 
                               const float3 lightcolor, 
                               float shadow)
{
    const float cosA = dot(viewdir, lightdir);
    return lightcolor * PF_HenyeyGreensteinDual(cosA, phase0, phase1, phaseW) * shadow;
}