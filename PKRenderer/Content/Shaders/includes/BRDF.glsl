#pragma once
#include "Utilities.glsl"
#include "Constants.glsl"

#define PK_DFG_USE_MULTIPLE_SCATTER 1

PK_DECLARE_SET_GLOBAL uniform sampler2D pk_PreIntegratedDFG;

struct BxDFSurf
{
    // Diffuse reflectance of the surface. Unlike albedo this contains metallic attenuation.
    float3 diffuse; 
    float3 F0;
    float3 normal;
    float3 view_dir;
    float3 subsurface;
    float3 Fsheen;
    float sheen_alpha;
    float clear_coat;
    float clear_coat_gloss;
    // actual roughness. roughness elsewhere is linear roughness
    float alpha; 
    float nv;
};


//----------IMPORTANCE SAMPLING UTILITIES----------//

// Source https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
float Futil_SpecularDominantFactor(float nv, float roughness)
{
   float alpha = pow2(roughness);
   return (1.0f - alpha) * (sqrt(1.0f - alpha) + alpha);
}

float3 Futil_SpecularDominantDirection(const float3 N, const float3 V, float roughness)
{
    const float factor = Futil_SpecularDominantFactor(abs(dot(N, V)), roughness);
    return normalize(lerp(N, reflect(-V, N), factor));
}

float2x3 Futil_SpecularDominantBasis(const float3 normal, const float3 view_dir, const float roughness, const float radius, inout float3 out_direction)
{
    out_direction = Futil_SpecularDominantDirection(normal, view_dir, roughness);
    const float3 l = reflect(-out_direction, normal);
    const float3 t = normalize(cross(normal,l));
    const float3 b = cross(l,t);
    return float2x3(t * radius, b * radius);
}

//Source: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf 
float Futil_SpecularLobeHalfAngle(const float roughness, const float volume_factor) 
{
    return atan(roughness * volume_factor / ( 1.0 - volume_factor)); 
}



//----------SURFACE SETUP UTILITIES----------//

float4 Futil_SampleIntegratedDFG(const float NoV, const float alpha) 
{ 
    return texture(pk_PreIntegratedDFG, float2(NoV, sqrt(alpha))); 
}

float3 Futil_ComputeF0(const float3 albedo, const float metallic, const float clear_coat) 
{
    float3 F0 = lerp(PK_DIELECTRIC_SPEC.rgb, albedo, metallic);
    #if defined(BxDF_ENABLE_CLEARCOAT)
        // IOR transfer approximation between clear coat & bottom layer. Assumes IOR of 1.5
        float3 F0_clear_coat = saturate(F0 * (F0 * (0.941892 - 0.263008 * F0) + 0.346479) - 0.0285998);
        F0 = lerp(F0, F0_clear_coat, clear_coat);
    #endif
    return F0;
}

float3 Futil_ComputeDiffuseColor(const float3 albedo, const float metallic) 
{ 
    return albedo * PK_DIELECTRIC_SPEC.a * (1.0f - metallic); 
}

float3 Futil_PremultiplyTransparency(float3 diffuse, const float metallic, inout float alpha)
{
    const float reflectivity = PK_DIELECTRIC_SPEC.r + metallic * PK_DIELECTRIC_SPEC.a;
    diffuse *= alpha;
    alpha = reflectivity + alpha * (1.0f - reflectivity);
    return diffuse;
}



//----------SPECULAR AREA LIGHT FUNCTIONS----------//

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

    float invLenH = inversesqrt(2 + 2 * VoL);
    NoH = saturate((NoL + NoV ) * invLenH);
    LoH = saturate(invLenH + invLenH * VoL);
}

// Sphere area light ggx D energy conservation factor
float Fsal_GGXFactor(float LoH, float alpha, float sina)
{
    const float a2 = pow2(alpha);
    const float a2s = 0.25f * sina * (3.0f * alpha * sina) / (LoH + 1e-2f);
    return a2 / (a2 + a2s);
}



//----------PROBABILITY DISTRIBUTION FUNCTIONS----------//

float Pdf_Lambert(const float3 normal, const float3 direction) 
{
    return max(0.0f, dot(normal, direction)) * PK_INV_PI;
}

float Pdf_GGXVNDF(const float3 Ve, const float3 Ne, const float alpha)
{
    const float NoV = Ve.z;
    const float NoH = Ne.z;
    const float VoH = dot(Ve, Ne);
    const float a2 = pow2(alpha);
    const float3 Hs = float3(alpha * Ne.xy, a2 * NoH);
    const float S = dot(Hs, Hs);
    const float D = PK_INV_PI * a2 * pow2(a2 / S);
    const float LenV = length(float3(alpha * Ve.xy, NoV));

    //Source: "Sampling Visible GGX Normals with Spherical Caps", Jonathan Dupuy & Anis Benyoub - High Performance Graphics 2023
    const float s = 1.0f + length(Ve.xy);
    const float s2 = s * s;
    const float k = (s2 - a2 * s2) / (s2 + a2 * pow2(Ve.z)); 

    return (2.0f * D * VoH) / (k * NoV + LenV);
}



//----------IMPORTANCE SAMPLING FUNCTIONS----------//

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

//Source: https://agraphicsguy.wordpress.com/2015/11/01/sampling-microfacet-brdf/
float3 Fr_Inverse_GGX(float2 Xi, float alpha)
{
    const float a2 = alpha * alpha;
    const float Phi = PK_TWO_PI * Xi.x;
    const float cosTheta = sqrt((1 - Xi.y) / (1 + (a2 - 1) * Xi.y));
    const float sinTheta = sqrt( 1 - pow2(cosTheta));

    float3 H;
    H.x = sinTheta * cos(Phi);
    H.y = sinTheta * sin(Phi);
    H.z = cosTheta;
    return H;
}

//Source: "Sampling the GGX Distribution of Visible Normals", Heitz
float3 Fr_Inverse_GGXVNDF(const float2 Xi, const float3 Ve, const float alpha)
{
    const float3 Vh = normalize(float3(alpha * Ve.xy, Ve.z));

    //Source: "Sampling Visible GGX Normals with Spherical Caps", Jonathan Dupuy & Anis Benyoub - High Performance Graphics 2023
    const float phi = PK_TWO_PI * Xi.x;
    const float s = 1.0f + length(Ve.xy);
    const float a2 = pow2(alpha);
    const float s2 = pow2(s);
    const float k = (s2 - a2 * s2) / (s2 + a2 * pow2(Ve.z));

    const float t2 = lerp(1.0f, -k * Vh.z, Xi.y);
    const float r = sqrt(saturate(1.0f - pow2(t2)));
    const float t0 = r * cos(phi);
    const float t1 = r * sin(phi);
    const float3 H = float3(t0, t1, t2) + Vh;
    return normalize(float3(alpha * H.xy, max(0.0f, H.z)));
}

float3 Fr_Inverse_GGXVNDF_Full(float2 Xi, const float3 normal, const float3 view_dir, const float roughness)
{
    // prevent grazing angles 
    Xi.x *= 0.98f;
    const float alpha = pow2(roughness);
    const float3x3 basis = make_TBN(normal);
    const float3 Ve = -view_dir * basis;
    const float3 Ne = Fr_Inverse_GGXVNDF(Xi, Ve, alpha);
    return basis * reflect(-Ve, Ne);
}



//----------BxDF COMMON FUNCTIONS----------//

float Fp_HenyeyGreenstein(float cosA, float g)
{
    const float gsq = pow2(g);
    const float denom = 1.0f + gsq - 2.0f * g * cosA;
    return PK_INV_FOUR_PI * (1.0f - gsq) * inversesqrt(pow3(denom));
}

float Fp_HenyeyGreensteinDual(float cosA, float g0, float g1, float w) 
{
    return lerp(Fp_HenyeyGreenstein(cosA, g0), Fp_HenyeyGreenstein(cosA, g1), w); 
}

float Fp_Schlick(float cosA, float g) 
{ 
    return PK_INV_FOUR_PI * (1.0f - pow2(g)) / pow2(1.0f - g * cosA); 
}


float F_Schlick(float F0, float F90, float cosA) 
{
    return F0 + (F90 - F0) * pow5(1.0f - cosA); 
}

float3 F_Schlick(float3 F0, float F90, float cosA) 
{
    return F0 + (F90 - F0) * pow5(1.0f - cosA); 
}

float3 F_IntegratedGGXDF(float3 F0, float F90, float3 Dfg) 
{
#if PK_DFG_USE_MULTIPLE_SCATTER
    return lerp(Dfg.xxx, Dfg.yyy, F0);
#else
    return (Dfg.x * F0 + F90 * DFG.y); 
#endif
}

float3 Fe_GGXEnergyCompensation(const float3 F0, const float3 Dfg)
{
#if PK_DFG_USE_MULTIPLE_SCATTER
    // See "Multiple-Scattering Microfacet BSDFs with the Smith Model"
    return 1.0f + F0 * (1.0f / Dfg.y - 1.0f);
#else
    return 1.0f.xxx;
#endif
}


float Fd_Wrap(float NoL, float w) 
{
    return saturate((NoL + w) / pow2(1.0f + w));
}

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

float V_SmithGGXCorrelated(float NoL, float NoV, float alpha)
{
    const float a2 = pow2(alpha);
    const float LambdaV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    const float LambdaL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    return 0.5f / (LambdaV + LambdaL);
}

// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
float V_Neubelt(float NoV, float NoL) 
{
    return 0.25f * (NoL + NoV - NoL * NoV); 
}

// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
float V_Kelemen(float LoH)
{
    return saturate(0.25f / pow2(LoH));
}

float D_GGX(float NoH, float alpha)
{
    const float a2 = pow2(alpha);
    const float d = (NoH * a2 - NoH) * NoH + 1.0f;
    return PK_INV_PI * a2 / (pow2(d) + 1e-7f);
}

// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
float D_Charlie(float NoH, float alpha)
{
    float inv_alpha  = inversesqrt(alpha);
    float cos_2h = NoH * NoH;
    float sin_2h = max(1.0 - cos_2h, 0.0078125);
    return (2.0 + inv_alpha) * pow(sin_2h, inv_alpha * 0.5) * PK_INV_TWO_PI;
}


//----------BxDFs----------//

float3 BxDF_SceneGI(const BxDFSurf surf, 
                    const float3 direction, 
                    const float3 radiance, 
                    const float3 gi_Ld, 
                    const float3 gi_Ls, 
                    const float  gi_LsFade)
{
    const float nl = max(0.0f, dot(surf.normal, direction));
    const float vl = dot(surf.view_dir, direction);
    const float ih = inversesqrt(2.0f + 2.0f * vl);
    const float nh = saturate((nl + surf.nv) * ih);
    const float lh = saturate(ih + ih * vl);
    const float3 integrated_dfg = Futil_SampleIntegratedDFG(surf.nv, surf.alpha).rgb; 

    float3 Lt = 0.0f.xxx;
    float3 Ls = 0.0f.xxx;

    // ray traced specular & diffuse. 
    {
        const float F90 = saturate(50.0f * surf.F0.g);
        const float3 E = Fe_GGXEnergyCompensation(surf.F0, integrated_dfg);
        Lt += F_IntegratedGGXDF(surf.F0, F90, integrated_dfg) * E * gi_Ls * gi_LsFade;
        Lt += surf.diffuse * gi_Ld * integrated_dfg.z;
    }

    // GGX bottom layer specular for rough surfaces.
    {
        const float V = V_SmithGGXCorrelated(nl, surf.nv, surf.alpha);
        const float D = D_GGX(nh, surf.alpha);
        const float Fr = max(0.0f, V * D  * nl);
        const float F90 = saturate(50.0f * surf.F0.g);
        const float3 E = Fe_GGXEnergyCompensation(surf.F0, integrated_dfg);
        // Completely nonsense approx fallback for gi spec. Should just be schlick fresnel...
        Ls += F_IntegratedGGXDF(surf.F0, F90, integrated_dfg) * Fr * E * (1.0f - gi_LsFade);
    }

    #if defined(BxDF_ENABLE_CLEARCOAT)
    {
        // Use higher roughness floor due to sh volatility.
        const float Rc = lerp(0.15f, 0.001f, surf.clear_coat_gloss);
        const float V = V_Kelemen(lh);
        const float D = D_GGX(nh, Rc);
        const float Fr = max(0.0f, V * D * nl);
        const float Fcc = F_Schlick(0.04f, 1.0f, lh) * surf.clear_coat;
        Lt *= 1.0f - Fcc;
        Ls += Fcc * Fr;
    }
    #endif

    #if defined(BxDF_ENABLE_SHEEN)
    {
        const float V = V_Neubelt(surf.nv, nl);
        const float D = D_Charlie(nh, surf.sheen_alpha);
        const float Fr = max(0.0f, V * D * nl);
        const float Fcc = Futil_SampleIntegratedDFG(surf.nv, surf.sheen_alpha).w;
        Lt *= 1.0f - Fcc * cmax(surf.Fsheen);
        Ls += surf.Fsheen * Fr;
    }
    #endif

    return Lt + Ls * radiance;
}

float3 BxDF_Principled(const BxDFSurf surf, const float3 direction, const float3 radiance, float shadow, float source_radius)
{
    const float nl = max(0.0f, dot(surf.normal, direction));
    const float vl = dot(surf.view_dir, direction);
    const float ih = inversesqrt(2.0f + 2.0f * vl);
    float nh = saturate((nl + surf.nv) * ih);
    float lh = saturate(ih + ih * vl);
    const float3 integrated_dfg = Futil_SampleIntegratedDFG(surf.nv, surf.alpha).xyz; 

    // Spherical area light tilt of LH & NH
    {
        source_radius = saturate(source_radius * (1.0f - pow2(surf.alpha)));
        Fsal_MaximumNHLH(source_radius, surf.nv, vl, nl, /*out*/ lh, /*out*/ nh);
    }

    float3 Lc = 0.0f.xxx;
    float3 Lt = 0.0f.xxx;

    // Diffuse
    {
        // Retroreflectivity fade out for spherical area lights
        const float FbW = 1.0f; //saturate(1.0f - sourceRadius / 0.2f);
        const float Fd = Fd_Chan(surf.nv, nl, nh, lh, FbW, surf.alpha);
        Lc += surf.diffuse * Fd * nl * PK_INV_PI; 
    }

    // Subsurface 
    #if defined(BxDF_ENABLE_SUBSURFACE)
    {
        const float subsurface_power = 16.0f;
        const float subsurface_distort = 0.1f;
        const float subsurface_thickness = 0.4f;
        const float subsurface_scale = 1.0f;
        
        // Source: https://colinbarrebrisebois.com/2012/04/09/approximating-translucency-revisited-with-simplified-spherical-gaussian/
        // Purely fictional. A better solution would be to calculate transmittance from shadow distance.
        const float scatter_VoH = saturate(dot(surf.view_dir, -(direction + surf.normal * subsurface_distort)));
        const float exp2_factor = subsurface_power * 1.4427f + 1.4427f;
        const float scatter_forward = exp2(scatter_VoH * exp2_factor - exp2_factor) * subsurface_scale;
        const float scatter_backward = pow(saturate(nl * 0.6666f + 0.333f), 1.5f) * (5.0f / 3.0f) * PK_INV_TWO_PI;
        const float3 Fss = surf.subsurface * lerp(scatter_backward, 1.0f, scatter_forward) * (1.0f - subsurface_thickness); 
        Lc *= 1.0f.xxx - Fss;
        Lt += Fss * PK_INV_PI;
    }
    #endif

    // GGX bottom layer specular
    {
        const float V = V_SmithGGXCorrelated(nl, surf.nv, surf.alpha);
        const float D = D_GGX(nh, surf.alpha);
        const float De = Fsal_GGXFactor(lh, surf.alpha, source_radius);
        const float Fr = max(0.0f, V * D * De * nl);
        const float F90 = saturate(50.0f * surf.F0.g);
        const float3 E = Fe_GGXEnergyCompensation(surf.F0, integrated_dfg);
        Lc += F_Schlick(surf.F0, F90, lh) * Fr * E;
    }

    #if defined(BxDF_ENABLE_CLEARCOAT)
    {
        // @TODO use geometry normal for clear coat.
        const float Rc = lerp(0.1f, 0.001f, surf.clear_coat_gloss);
        const float V = V_Kelemen(nh);
        const float D = D_GGX(nh, Rc);
        const float De = Fsal_GGXFactor(lh, Rc, source_radius);
        const float Fr = max(0.0f, V * D * De * nl);
        const float Fcc = F_Schlick(0.04f, 1.0f, lh) * surf.clear_coat;
        Lc *= 1.0f - Fcc;
        Lt *= 1.0f - Fcc;
        Lc += Fcc * Fr;
    }
    #endif

    #if defined(BxDF_ENABLE_SHEEN)
    {
        const float V = V_Neubelt(surf.nv, nl);
        const float D = D_Charlie(nh, surf.sheen_alpha);
        const float Fr = max(0.0f, V * D * nl);
        const float Fcc = Futil_SampleIntegratedDFG(surf.nv, surf.sheen_alpha).w;
        Lc *= 1.0f - Fcc * cmax(surf.Fsheen);
        Lt *= 1.0f - Fcc * cmax(surf.Fsheen);
        Lc += surf.Fsheen * Fr;
    }
    #endif

    return Lc * radiance * shadow +
           Lt * radiance * 1.0f; /*transmittanceShadow*/
}

float3 BxDF_FullyRoughMinimal(const BxDFSurf surf, const float3 direction, const float3 radiance, float shadow, float angle)
{
    const float nl = dot(direction, surf.normal);
    
    // Regain some energy for metallic surfaces so that they get some voxel intensity.
    float3 Fd = surf.diffuse + surf.F0 * 0.45f;

    #if defined(BxDF_ENABLE_SUBSURFACE)
        // Cheap subsurface scatter
        Fd *= Fd_Wrap(nl, 0.5f);
        Fd *= saturate(surf.subsurface + max(0.0f, nl) * shadow);
    #else
        Fd *= max(0.0f, nl) * shadow;
    #endif
    
    return Fd * radiance * PK_INV_PI;
}

float3 BxDF_Volumetric(const float3 view_dir, 
                       const float phase0, 
                       const float phase1, 
                       const float phaseW, 
                       const float3 direction, 
                       const float3 radiance, 
                       float shadow)
{
    const float cosA = dot(view_dir, direction);
    return radiance * Fp_HenyeyGreensteinDual(cosA, phase0, phase1, phaseW) * shadow;
}
