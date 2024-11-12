#pragma once
#include "PostFXResources.glsl"
#include "Encoding.glsl"
#include "Constants.glsl"

const float3x3 LIN_2_LMS_MAT = float3x3
(
    3.90405e-1, 5.49941e-1, 8.92632e-3,
    7.08416e-2, 9.63172e-1, 1.35775e-3,
    2.31082e-2, 1.28021e-1, 9.36245e-1
);

const float3x3 LMS_2_LIN_MAT = float3x3
(
     2.85847e+0, -1.62879e+0, -2.48910e-2,
    -2.10182e-1,  1.15820e+0,  3.24281e-4,
    -4.18120e-2, -1.18169e-1,  1.06867e+0
);

float3 Tonemap_ACESFilm(float3 color, float exposure)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    color *= exposure;
    return (color * (a * color + b)) / (color * (c * color + d) + e);
}

float3 Tonemap_HejlDawson(float3 color, float exposure)
{
    const float a = 6.2f;
    const float b = 0.5f;
    const float c = 1.7f;
    const float d = 0.06f;

    color *= exposure;
    color = max(0.0f.xxx, color - 0.004f);
    color = (color * (a * color + b)) / (color * (a * color + c) + d);
    return color * color;
}

float Tonemap_Uchimura(float x, float P, float a, float m, float l, float c, float b) 
{
    // Uchimura 2017, "HDR theory and practice"
    // Math: https://www.desmos.com/calculator/gslcdxvipg
    // Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float w0 = 1.0 - smoothstep(0.0, m, x);
    float w2 = step(m + l0, x);
    float w1 = 1.0 - w0 - w2;

    float T = m * pow(x / m, c) + b;
    float S = P - (P - S1) * exp(CP * (x - S0));
    float L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float3 Tonemap_Uchimura(float3 color, float exposure) 
{
    const float P = 1.0;  // max display brightness
    const float a = 1.0;  // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4;  // linear section length
    const float c = 1.33; // black
    const float b = 0.0;  // pedestal

    color *= exposure;

    return float3
    (
        Tonemap_Uchimura(color.r, P, a, m, l, c, b),
        Tonemap_Uchimura(color.g, P, a, m, l, c, b),
        Tonemap_Uchimura(color.b, P, a, m, l, c, b)
    );
}

float3 Tonemap_Lottes(float3 color, float exposure) 
{
    // Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
    const float a = 1.6f;
    const float d = 0.977f;
    const float hm = 8.0f; // hdr max
    const float mi = 0.18f; // Mid in
    const float mo = 0.267f; // Mid out

    // Can be precomputed
    const float b = (-pow(mi, a) + pow(hm, a) * mo) / ((pow(hm, a * d) - pow(mi, a * d)) * mo);
    const float c = (pow(hm, a * d) * pow(mi, a) - pow(hm, a) * pow(mi, a * d) * mo) / ((pow(hm, a * d) - pow(mi, a * d)) * mo);
    
    color *= exposure;

    return pow(color, a.xxx) / (pow(color, a.xxx * d) * b + c);
}

// Source: https://iolite-engine.com/blog_posts/minimal_agx_implementation
float3 Tonemap_AgX(float3 color, float exposure) 
{
    const float3x3 agx_mat = float3x3(
        0.842479062253094, 0.0423282422610123, 0.0423756549057051,
        0.0784335999999992,  0.878468636469772,  0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104);
      
    const float min_ev = -12.47393f;
    const float max_ev = 4.026069f;

    // Input transform (inset)
    color = agx_mat * (color * exposure);
    
    // Log2 space encoding
    color = clamp(log2(color), min_ev, max_ev);
    color = (color - min_ev) / (max_ev - min_ev);
    
    // Apply sigmoid function approximation
    const float3 x2 = color * color;
    const float3 x4 = x2 * x2;
    
    color = + 15.5   * x4 * x2
            - 40.14  * x4 * color
            + 31.96  * x4
            - 6.868  * x2 * color
            + 0.4298 * x2
            + 0.1191 * color
            - 0.00232;

    const mat3 agx_mat_inv = mat3(
        1.19687900512017, -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    
    // Inverse input transform (outset)
    color = agx_mat_inv * color;
  
    // sRGB IEC 61966-2-1 2.2 Exponent Reference EOTF Display
    // NOTE: We're linearizing the output here. Comment/adjust when
    // *not* using a sRGB render target
    color = pow(color, 2.2f.xxx);

    return color;
}

float3 Tonemap_LUT(float3 color, float exposure) 
{
    const float3 stimulus = color * exposure;
    const float3 encoded = stimulus / (stimulus + 1.0);
    const float3 size = textureSize(pk_Tonemap_LutTex, 0).xyz;
    return texture(pk_Tonemap_LutTex, encoded * ((size - 1.0) / size) + 0.5f / size).rgb;
}

float3 Saturate_BT2100(float3 color, float amount) { return lerp_true(dot(color, PK_LUMA_BT2100).xxx, color, amount); }

float3 Saturate_BT709(float3 color, float amount) { return lerp_true(dot(color, PK_LUMA_BT709).xxx, color, amount); }

float3 Saturate_Rec601(float3 color, float amount) { return lerp_true(dot(color, PK_LUMA_REC601).xxx, color, amount); }

float Vignette(float2 uv)
{
    uv *= 1.0 - uv.yx;   
    return min(1.0f, pow(uv.x * uv.y * pk_Vignette_Intensity, pk_Vignette_Power)); 
}

float3 LinearToGamma(float3 color)
{
    //Source: http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
    float3 S1 = sqrt(color);
    float3 S2 = sqrt(S1);
    float3 S3 = sqrt(S2);
    return 0.662002687 * S1 + 0.684122060 * S2 - 0.323583601 * S3 - 0.0225411470 * color;
}

float3 ApplyColorGrading(float3 color)
{
    float3 final = saturate(color);

    // White balance
    float3 lms = LIN_2_LMS_MAT * final;
    lms *= pk_CC_WhiteBalance.xyz;
    final = LMS_2_LIN_MAT * lms;

    // Lift/gamma/gain
    final = max(final, 0.0);
    final = pk_CC_Gain.xyz * (pk_CC_Lift.xyz * (1.0 - final) + pow(final, pk_CC_Gamma.xyz));

    // Hue/saturation/value
    float3 hsv = RGBToHSV(final);
    hsv.x = mod(hsv.x + pk_CC_HSV.x, 1.0);
    hsv.yz *= pk_CC_HSV.yz;
    final = saturate(HSVToRGB(hsv));
    
    // Vibrance
    const float sat = max(final.r, max(final.g, final.b)) - min(final.r, min(final.g, final.b));
    final = lerp(dot(final, PK_LUMA_BT709).xxx, final, (1.0 + (pk_CC_Vibrance * (1.0 - (sign(pk_CC_Vibrance) * sat)))));
    
    // Contrast
    final = saturate((final - 0.5) * pk_CC_LumaContrast + 0.5);

    // Gain
    const float g = pk_CC_LumaGain;
    const float f = pow(2.0, g) * 0.5;
    final.r = final.r < 0.5f ? pow(final.r, g) * f : 1.0f - pow(1.0f - final.r, g) * f;
    final.g = final.g < 0.5f ? pow(final.g, g) * f : 1.0f - pow(1.0f - final.g, g) * f;
    final.b = final.b < 0.5f ? pow(final.b, g) * f : 1.0f - pow(1.0f - final.b, g) * f;

    // Gamma
    final = pow(final, pk_CC_LumaGamma.xxx);

    // Color mixer
    final = float3
    (
        dot(final, pk_CC_MixRed.rgb),
        dot(final, pk_CC_MixGreen.rgb),
        dot(final, pk_CC_MixBlue.rgb)
    );

    return lerp(color, final, pk_CC_Contribution);
}

float3 ApplyLutColorGrading(float3 color)
{
    const float3 uvw = saturate(color);
    const float3 final = texture(pk_CC_LutTex, uvw).rgb;
    const float contribution = pk_CC_Contribution;
    return lerp(color, final, contribution);
}
