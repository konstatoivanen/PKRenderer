#pragma once
#include GBuffers.glsl

PK_DECLARE_LOCAL_CBUFFER(pk_DoF_Params)
{
    float pk_DoF_FocalLength;
    float pk_DoF_FNumber;
    float pk_DoF_FilmHeight;
    float pk_DoF_FocusSpeed;
    float pk_DoF_MaximumCoC;
};

struct AutoFocusData
{
    float Distance;
    float LensCoefficient;
};

PK_DECLARE_VARIABLE(AutoFocusData, pk_DoF_AutoFocusParams, PK_SET_PASS);

float GetLensCoefficient(float focusDistance) { return pk_DoF_FocalLength * pk_DoF_FocalLength / (pk_DoF_FNumber * (focusDistance - pk_DoF_FocalLength) * pk_DoF_FilmHeight * 2); }

float GetLensCoefficient() { return PK_VARIABLE_DATA(pk_DoF_AutoFocusParams).LensCoefficient; }

float GetFocusDistance() { return PK_VARIABLE_DATA(pk_DoF_AutoFocusParams).Distance;  }

float GetCircleOfConfusion01(float viewDepth)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_DoF_AutoFocusParams);
    return min(1.0f, abs(viewDepth - data.Distance) * data.LensCoefficient / viewDepth / pk_DoF_MaximumCoC);
}

float4 GetCirclesOfConfusion01(float4 viewDepths)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_DoF_AutoFocusParams);
    return min(float4(1.0f), abs(viewDepths - data.Distance) * data.LensCoefficient / viewDepths / pk_DoF_MaximumCoC);
}

float GetCircleOfConfusion(float viewDepth)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_DoF_AutoFocusParams);
    return clamp((viewDepth - data.Distance) * data.LensCoefficient / viewDepth, -pk_DoF_MaximumCoC, pk_DoF_MaximumCoC);
}

float4 GetCirclesOfConfusion(float4 viewDepths)
{
    AutoFocusData data = PK_VARIABLE_DATA(pk_DoF_AutoFocusParams);
    return clamp((viewDepths - data.Distance) * data.LensCoefficient / viewDepths, -pk_DoF_MaximumCoC, pk_DoF_MaximumCoC);
}
