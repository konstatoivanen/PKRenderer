#pragma once

#define PK_PI               3.1415926535f
#define PK_TWO_PI           6.2831853071f
#define PK_FOUR_PI          12.566370614f
#define PK_INV_PI           0.3183098861f
#define PK_INV_TWO_PI       0.1591549430f
#define PK_INV_FOUR_PI      0.0795774715f
#define PK_HALF_PI          1.5707963267f
#define PK_INV_HALF_PI      0.6366197723f
#define PK_SQRT_PI          1.7724538509f
#define PK_TWO_SQRT2        2.8284271247f
#define PK_SQRT2            1.4142135623f
#define PK_INV_SQRT2        0.7071067811f
#define PK_HALF_MAX         65504.0
#define PK_HALF_MAX_MINUS1  65472.0

#define pk_Grey vec4(0.214041144, 0.214041144, 0.214041144, 0.5)
// standard dielectric reflectivity coef at incident angle (= 4%)
#define pk_DielectricSpecular vec4(0.04, 0.04, 0.04, 1.0 - 0.04) 
#define pk_Luminance vec4(0.2126729, 0.7151522, 0.0721750, 1.0f) //float4(0.0396819152, 0.458021790, 0.00609653955, 1.0)

#define pk_L1Basis float4(0.282095f, 0.488603f.xxx)
#define pk_L1Basis_Cosine float4(0.88622692545f, 1.02332670795.xxx)
#define pk_L1Basis_Irradiance float4(3.141593f, 2.094395f.xxx)