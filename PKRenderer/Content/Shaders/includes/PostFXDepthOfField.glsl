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

PK_DECLARE_VARIABLE(DoFAutoFocusState, pk_DoF_AutoFocusState);

float DoF_GetLensCoefficient() 
{ 
    return PK_VARIABLE_DATA(pk_DoF_AutoFocusState).lens_coefficient; 
}

float DoF_GetFocusDepth() 
{ 
    return PK_VARIABLE_DATA(pk_DoF_AutoFocusState).focus_depth;  
}

float DoF_GetCircleOfConfusion01(float view_depth)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return min(1.0f, abs(view_depth - state.focus_depth) * state.lens_coefficient / view_depth / pk_DoF_MaximumCoC);
}

float4 DoF_GetCirclesOfConfusion01(float4 view_depths)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return min(float4(1.0f), abs(view_depths - state.focus_depth) * state.lens_coefficient / view_depths / pk_DoF_MaximumCoC);
}

float DoF_GetCircleOfConfusion(float view_depth)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return clamp((view_depth - state.focus_depth) * state.lens_coefficient / view_depth, -pk_DoF_MaximumCoC, pk_DoF_MaximumCoC);
}

float4 DoF_GetCirclesOfConfusion(float4 view_depths)
{
    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
    return clamp((view_depths - state.focus_depth) * state.lens_coefficient / view_depths, -pk_DoF_MaximumCoC, pk_DoF_MaximumCoC);
}
