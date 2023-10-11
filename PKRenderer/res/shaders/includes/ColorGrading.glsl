#pragma once
#include SharedPostEffects.glsl
#include Encoding.glsl
#include Constants.glsl

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

float3 Saturate_BT2100(float3 color, float amount) 
{
	const float grayscale = dot(color, float3(0.2627f, 0.6780f, 0.0593f));
	return lerp_true(grayscale.xxx, color, amount);
}

float3 Saturate_BT709(float3 color, float amount) 
{
	const float grayscale = dot(color, float3(0.2126729f, 0.7151522f, 0.0721750f));
	return lerp_true(grayscale.xxx, color, amount);
}

float3 Saturate_Rec601(float3 color, float amount) 
{
	const float grayscale = dot(color, float3(0.3f, 0.59f, 0.11f));
	return lerp_true(grayscale.xxx, color, amount);
}

float Vignette(float2 uv)
{
    uv *= 1.0 - uv.yx;   
    return min(1.0f, pow(uv.x * uv.y * pk_VignetteGrain.x, pk_VignetteGrain.y)); 
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

    const float contrast = pk_ContrastGainGammaContribution.x;
    const float gain = pk_ContrastGainGammaContribution.y;
    const float gamma = pk_ContrastGainGammaContribution.z;
    const float contribution = pk_ContrastGainGammaContribution.w;

    // White balance
    float3 lms = mul(LIN_2_LMS_MAT, final);
    lms *= pk_WhiteBalance.xyz;
    final = mul(LMS_2_LIN_MAT, lms);

    // Lift/gamma/gain
    final = max(final, 0.0);
    final = pk_Gain.xyz * (pk_Lift.xyz * (1.0 - final) + pow(final, pk_Gamma.xyz));

    // Hue/saturation/value
    float3 hsv = RGBToHSV(final);
    hsv.x = mod(hsv.x + pk_HSV.x, 1.0);
    hsv.yz *= pk_HSV.yz;
    final = saturate(HSVToRGB(hsv));
    
    // Vibrance
    float sat = max(final.r, max(final.g, final.b)) - min(final.r, min(final.g, final.b));
    final = lerp(dot(final, pk_Luminance.xyz).xxx, final, (1.0 + (pk_Vibrance * (1.0 - (sign(pk_Vibrance) * sat)))));
    
    // Contrast
    final = saturate((final - 0.5) * contrast + 0.5);

    // Gain
    float f = pow(2.0, gain) * 0.5;
    final.r = final.r < 0.5f ? pow(final.r, gain) * f : 1.0f - pow(1.0f - final.r, gain) * f;
    final.g = final.g < 0.5f ? pow(final.g, gain) * f : 1.0f - pow(1.0f - final.g, gain) * f;
    final.b = final.b < 0.5f ? pow(final.b, gain) * f : 1.0f - pow(1.0f - final.b, gain) * f;

    // Gamma
    final = pow(final, gamma.xxx);

    // Color mixer
    final = float3
    (
        dot(final, pk_ChannelMixerRed.rgb),
        dot(final, pk_ChannelMixerGreen.rgb),
        dot(final, pk_ChannelMixerBlue.rgb)
    );

    return lerp(color, final, contribution);
}

float3 ApplyLutColorGrading(float3 color)
{
    const float3 uvw = saturate(color);
    const float3 final = tex2D(pk_ColorGradingLutTex, uvw).rgb;
    const float contribution = pk_ContrastGainGammaContribution.w;
    return lerp(color, final, contribution);
}