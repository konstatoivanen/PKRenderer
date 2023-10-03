#pragma once
#include GBuffers.glsl

PK_DECLARE_LOCAL_CBUFFER(pk_DofParams)
{
    float pk_FocalLength;
    float pk_FNumber;
    float pk_FilmHeight;
    float pk_FocusSpeed;
    float pk_MaximumCoC;
};

struct AutoFocusData
{
    float Distance;
    float LensCoefficient;
};

PK_DECLARE_VARIABLE(AutoFocusData, pk_AutoFocusParams, PK_SET_PASS);

float GetLensCoefficient(float focusDistance) { return pk_FocalLength * pk_FocalLength / (pk_FNumber * (focusDistance - pk_FocalLength) * pk_FilmHeight * 2); }

float GetLensCoefficient() { return PK_VARIABLE_DATA(pk_AutoFocusParams).LensCoefficient; }

float GetFocusDistance() { return PK_VARIABLE_DATA(pk_AutoFocusParams).Distance;  }

float GetCircleOfConfusion01(float viewDepth)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_AutoFocusParams);
    return min(1.0f, abs(viewDepth - data.Distance) * data.LensCoefficient / viewDepth / pk_MaximumCoC);
}

float4 GetCirclesOfConfusion01(float4 viewDepths)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_AutoFocusParams);
    return min(float4(1.0f), abs(viewDepths - data.Distance) * data.LensCoefficient / viewDepths / pk_MaximumCoC);
}

float GetCircleOfConfusion(float viewDepth)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_AutoFocusParams);
    return clamp((viewDepth - data.Distance) * data.LensCoefficient / viewDepth, -pk_MaximumCoC, pk_MaximumCoC);
}

float4 GetCirclesOfConfusion(float4 viewDepths)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_AutoFocusParams);
    return clamp((viewDepths - data.Distance) * data.LensCoefficient / viewDepths, -pk_MaximumCoC, pk_MaximumCoC);
}
