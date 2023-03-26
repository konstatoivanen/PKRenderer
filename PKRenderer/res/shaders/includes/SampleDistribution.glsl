#pragma once

// Poisson disk kernels
const float2 PK_POISSON_DISK_16[16] = 
{
    float2(0,0),
    float2(0.54545456,0),
    float2(0.16855472,0.5187581),
    float2(-0.44128203,0.3206101),
    float2(-0.44128197,-0.3206102),
    float2(0.1685548,-0.5187581),
    float2(1,0),
    float2(0.809017,0.58778524),
    float2(0.30901697,0.95105654),
    float2(-0.30901703,0.9510565),
    float2(-0.80901706,0.5877852),
    float2(-1,0),
    float2(-0.80901694,-0.58778536),
    float2(-0.30901664,-0.9510566),
    float2(0.30901712,-0.9510565),
    float2(0.80901694,-0.5877853),
};

const float2 PK_POISSON_DISK_22[22] = 
{
    float2(0,0),
    float2(0.53333336,0),
    float2(0.3325279,0.4169768),
    float2(-0.11867785,0.5199616),
    float2(-0.48051673,0.2314047),
    float2(-0.48051673,-0.23140468),
    float2(-0.11867763,-0.51996166),
    float2(0.33252785,-0.4169769),
    float2(1,0),
    float2(0.90096885,0.43388376),
    float2(0.6234898,0.7818315),
    float2(0.22252098,0.9749279),
    float2(-0.22252095,0.9749279),
    float2(-0.62349,0.7818314),
    float2(-0.90096885,0.43388382),
    float2(-1,0),
    float2(-0.90096885,-0.43388376),
    float2(-0.6234896,-0.7818316),
    float2(-0.22252055,-0.974928),
    float2(0.2225215,-0.9749278),
    float2(0.6234897,-0.7818316),
    float2(0.90096885,-0.43388376),
};

const float2 PK_POISSON_DISK_43[43] = 
{
    float2(0,0),
    float2(0.36363637,0),
    float2(0.22672357,0.28430238),
    float2(-0.08091671,0.35451925),
    float2(-0.32762504,0.15777594),
    float2(-0.32762504,-0.15777591),
    float2(-0.08091656,-0.35451928),
    float2(0.22672352,-0.2843024),
    float2(0.6818182,0),
    float2(0.614297,0.29582983),
    float2(0.42510667,0.5330669),
    float2(0.15171885,0.6647236),
    float2(-0.15171883,0.6647236),
    float2(-0.4251068,0.53306687),
    float2(-0.614297,0.29582986),
    float2(-0.6818182,0),
    float2(-0.614297,-0.29582983),
    float2(-0.42510656,-0.53306705),
    float2(-0.15171856,-0.66472363),
    float2(0.1517192,-0.6647235),
    float2(0.4251066,-0.53306705),
    float2(0.614297,-0.29582983),
    float2(1,0),
    float2(0.9555728,0.2947552),
    float2(0.82623875,0.5633201),
    float2(0.6234898,0.7818315),
    float2(0.36534098,0.93087375),
    float2(0.07473,0.9972038),
    float2(-0.22252095,0.9749279),
    float2(-0.50000006,0.8660254),
    float2(-0.73305196,0.6801727),
    float2(-0.90096885,0.43388382),
    float2(-0.98883086,0.14904208),
    float2(-0.9888308,-0.14904249),
    float2(-0.90096885,-0.43388376),
    float2(-0.73305184,-0.6801728),
    float2(-0.4999999,-0.86602545),
    float2(-0.222521,-0.9749279),
    float2(0.07473029,-0.99720377),
    float2(0.36534148,-0.9308736),
    float2(0.6234897,-0.7818316),
    float2(0.8262388,-0.56332),
    float2(0.9555729,-0.29475483),
};

const float2 PK_POISSON_DISK_71[71] = 
{
    float2(0,0),
    float2(0.2758621,0),
    float2(0.1719972,0.21567768),
    float2(-0.061385095,0.26894566),
    float2(-0.24854316,0.1196921),
    float2(-0.24854316,-0.11969208),
    float2(-0.061384983,-0.2689457),
    float2(0.17199717,-0.21567771),
    float2(0.51724136,0),
    float2(0.46601835,0.22442262),
    float2(0.32249472,0.40439558),
    float2(0.11509705,0.50427306),
    float2(-0.11509704,0.50427306),
    float2(-0.3224948,0.40439552),
    float2(-0.46601835,0.22442265),
    float2(-0.51724136,0),
    float2(-0.46601835,-0.22442262),
    float2(-0.32249463,-0.40439564),
    float2(-0.11509683,-0.5042731),
    float2(0.11509732,-0.504273),
    float2(0.32249466,-0.40439564),
    float2(0.46601835,-0.22442262),
    float2(0.7586207,0),
    float2(0.7249173,0.22360738),
    float2(0.6268018,0.4273463),
    float2(0.47299224,0.59311354),
    float2(0.27715522,0.7061801),
    float2(0.056691725,0.75649947),
    float2(-0.168809,0.7396005),
    float2(-0.3793104,0.65698475),
    float2(-0.55610836,0.51599306),
    float2(-0.6834936,0.32915324),
    float2(-0.7501475,0.113066405),
    float2(-0.7501475,-0.11306671),
    float2(-0.6834936,-0.32915318),
    float2(-0.5561083,-0.5159932),
    float2(-0.37931028,-0.6569848),
    float2(-0.16880904,-0.7396005),
    float2(0.056691945,-0.7564994),
    float2(0.2771556,-0.7061799),
    float2(0.47299215,-0.59311366),
    float2(0.62680185,-0.4273462),
    float2(0.72491735,-0.22360711),
    float2(1,0),
    float2(0.9749279,0.22252093),
    float2(0.90096885,0.43388376),
    float2(0.7818315,0.6234898),
    float2(0.6234898,0.7818315),
    float2(0.43388364,0.9009689),
    float2(0.22252098,0.9749279),
    float2(0,1),
    float2(-0.22252095,0.9749279),
    float2(-0.43388385,0.90096885),
    float2(-0.62349,0.7818314),
    float2(-0.7818317,0.62348956),
    float2(-0.90096885,0.43388382),
    float2(-0.9749279,0.22252093),
    float2(-1,0),
    float2(-0.9749279,-0.22252087),
    float2(-0.90096885,-0.43388376),
    float2(-0.7818314,-0.6234899),
    float2(-0.6234896,-0.7818316),
    float2(-0.43388346,-0.900969),
    float2(-0.22252055,-0.974928),
    float2(0,-1),
    float2(0.2225215,-0.9749278),
    float2(0.4338835,-0.90096897),
    float2(0.6234897,-0.7818316),
    float2(0.78183144,-0.62348986),
    float2(0.90096885,-0.43388376),
    float2(0.9749279,-0.22252086),
};

const float3 PK_HAMMERSLEY_SET_5[5] =
{
    float3(1.0f,       0.0f,       0.0f),
    float3(0.309017f,  0.9510565f, 0.5f),
    float3(-0.8090171f, 0.5877852f, 0.25f),
    float3(-0.8090169f,-0.5877854f, 0.75f),
    float3(0.3090171f,-0.9510565f, 0.125f),
};

const float3 PK_HAMMERSLEY_SET_16[16] = 
{
    float3(1.0f,0.0f,0.0f),
    float3(0.9238795f,0.3826835f,0.5f),
    float3(0.7071068f,0.7071068f,0.25f),
    float3(0.3826834f,0.9238795f,0.75f),
    float3(-4.371139E-08f,1.0f,0.125f),
    float3(-0.3826835f,0.9238795f,0.625f),
    float3(-0.7071068f,0.7071068f,0.375f),
    float3(-0.9238796f,0.3826833f,0.875f),
    float3(-1.0f,-8.742278E-08f,0.0625f),
    float3(-0.9238795f,-0.3826834f,0.5625f),
    float3(-0.7071066f,-0.7071069f,0.3125f),
    float3(-0.3826831f,-0.9238797f,0.8125f),
    float3(1.192488E-08f,-1.0f,0.1875f),
    float3(0.3826836f,-0.9238794f,0.6875f),
    float3(0.707107f,-0.7071065f,0.4375f),
    float3(0.9238796f,-0.3826834f,0.9375f)
};

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i % N) / float(N), RadicalInverse_VdC(i));
}

float3 GetSampleDirectionHammersLey(const float3 Xi, float blur)
{
    float theta = (1.0f - Xi.z) / (1.0f + (blur - 1.0f) * Xi.z);
    float2 sincos = sqrt(float2(1.0f - theta, theta));
    return normalize(float3(Xi.xy * sincos.xx, sincos.y));
}

float3x3 ComposeTBN(float3 N, float3 U)
{
    float3 T = normalize(cross(U, N));
    float3 B = cross(N, T);
    return float3x3(T, B, N);
}

float3x3 ComposeTBN(float3 N)
{
    float3 U = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    return ComposeTBN(N, U);
}

float3 DistributeGGX(const float2 Xi, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * 3.14159265 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    return H;
}

float3 ImportanceSampleGGX(const float2 Xi, const float3 N, float roughness)
{
    float3 H = DistributeGGX(Xi, roughness);
    return normalize(ComposeTBN(N) * H);
}

float3 ImportanceSampleGGX(uint i, uint s, const float3 N, const float R)
{
    float2 Xi = Hammersley(i,s);
    return ImportanceSampleGGX(Xi, N, R);
}

float3 ImportanceSampleGGX(const float2 Xi, const float3 N, const float3 V, const float R)
{
    float3 H = DistributeGGX(Xi, R);
    float3 RV = reflect(V, N);
    float3x3 TBN = ComposeTBN(RV, N); 
    float3 D = TBN * H;

    // Flip sample along V/N plane to avoid self intersections
    // This also somewhat produces view dependent roughness as
    // directions align in narrow angles
    if (dot(D, N) < 0)
    {
        D = TBN * float3(H.x, -H.y, H.z);
    }

    return D;
}

float3 GetSampleDirectionSE(float3 worldNormal, uint index, const float sampleCount, float dither, float angle)
{
    float fi = float(index) + dither;
    float fiN = fi / sampleCount;
    float longitude = angle * fi;
    float latitude = asin(fiN * 2.0 - 1.0);

    float3 kernel;
    kernel.x = cos(latitude) * cos(longitude);
    kernel.z = cos(latitude) * sin(longitude);
    kernel.y = sin(latitude);
    kernel = faceforward(kernel, kernel, -worldNormal);
    return normalize(kernel);
}