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

PK_DECLARE_ATOMIC_VARIABLE(AutoFocusData, pk_AutoFocusParams, PK_SET_PASS);

float GetLensCoefficient(float focusDistance)
{
    return pk_FocalLength * pk_FocalLength / (pk_FNumber * (focusDistance - pk_FocalLength) * pk_FilmHeight * 2);
}

float GetLensCoefficient() 
{ 
    return PK_ATOMIC_DATA(pk_AutoFocusParams).LensCoefficient;
}

float GetFocusDistance() 
{ 
    return PK_ATOMIC_DATA(pk_AutoFocusParams).Distance; 
}

float GetCircleOfConfusion01(float linearDepth)
{
    AutoFocusData data = PK_ATOMIC_DATA(pk_AutoFocusParams);
    return min(1.0f, abs(linearDepth - data.Distance) * data.LensCoefficient / linearDepth / pk_MaximumCoC);
}

float4 GetCirclesOfConfusion01(float4 linearDepths)
{
    AutoFocusData data = PK_ATOMIC_DATA(pk_AutoFocusParams);
    return min(float4(1.0f), abs(linearDepths - data.Distance) * data.LensCoefficient / linearDepths / pk_MaximumCoC);
}

float GetCircleOfConfusion(float linearDepth)
{
    AutoFocusData data = PK_ATOMIC_DATA(pk_AutoFocusParams);
    return clamp((linearDepth - data.Distance) * data.LensCoefficient / linearDepth, -pk_MaximumCoC, pk_MaximumCoC);
}

float4 GetCirclesOfConfusion(float4 linearDepths)
{
    AutoFocusData data = PK_ATOMIC_DATA(pk_AutoFocusParams);
    return clamp((linearDepths - data.Distance) * data.LensCoefficient / linearDepths, -pk_MaximumCoC, pk_MaximumCoC);
}
