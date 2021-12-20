#pragma once

#define PK_PI            3.14159265359f
#define PK_TWO_PI        6.28318530718f
#define PK_FOUR_PI       12.56637061436f
#define PK_INV_PI        0.31830988618f
#define PK_INV_TWO_PI    0.15915494309f
#define PK_INV_FOUR_PI   0.07957747155f
#define PK_HALF_PI       1.57079632679f
#define PK_INV_HALF_PI   0.636619772367f
#define PK_TWO_SQRT2     2.828427
#define PK_SQRT2         1.414213
#define PK_INV_SQRT2     0.707106

#define pk_Grey vec4(0.214041144, 0.214041144, 0.214041144, 0.5)
// standard dielectric reflectivity coef at incident angle (= 4%)
#define pk_DielectricSpecular vec4(0.04, 0.04, 0.04, 1.0 - 0.04) 
#define pk_Luminance vec4(0.2125, 0.7154, 0.0721, 1.0f) //float4(0.0396819152, 0.458021790, 0.00609653955, 1.0)