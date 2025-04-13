#pragma once
#include "GBuffers.glsl"

PK_DECLARE_LOCAL_CBUFFER(pk_DoF_Params)
{
    float pk_DoF_FocalLength;
    float pk_DoF_FNumber;
    float pk_DoF_FilmHeight;
    float pk_DoF_FocusSpeed;
    float pk_DoF_MaximumCoC;
};

struct DoFAutoFocusState
{
    float focus_depth;
    float lens_coefficient;
};

PK_DECLARE_VARIABLE(DoFAutoFocusState, pk_DoF_AutoFocusState, PK_SET_GLOBAL);

float DoF_GetLensCoefficient() 
{ 
    return PK_VARIABLE_DATA(pk_DoF_AutoFocusState).lens_coefficient; 
}

float DoF_GetFocusDepth() 
{ 
    return PK_VARIABLE_DATA(pk_DoF_AutoFocusState).focus_depth;  
}

float DoF_GetCircleOfConfusion01(float viewDepth)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return min(1.0f, abs(viewDepth - state.focus_depth) * state.lens_coefficient / viewDepth / pk_DoF_MaximumCoC);
}

float4 DoF_GetCirclesOfConfusion01(float4 viewDepths)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return min(float4(1.0f), abs(viewDepths - state.focus_depth) * state.lens_coefficient / viewDepths / pk_DoF_MaximumCoC);
}

float DoF_GetCircleOfConfusion(float viewDepth)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return clamp((viewDepth - state.focus_depth) * state.lens_coefficient / viewDepth, -pk_DoF_MaximumCoC, pk_DoF_MaximumCoC);
}

float4 DoF_GetCirclesOfConfusion(float4 viewDepths)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return clamp((viewDepths - state.focus_depth) * state.lens_coefficient / viewDepths, -pk_DoF_MaximumCoC, pk_DoF_MaximumCoC);
}
