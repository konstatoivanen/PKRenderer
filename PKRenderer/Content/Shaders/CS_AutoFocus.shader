
#pragma pk_program SHADER_STAGE_COMPUTE AutoFocusCs

#include "includes/PostFXDepthOfField.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void AutoFocusCs()
{
    float center_depth = min(SampleMinZ(float2(0.5f, 0.5f), 4), pk_ClipParams.y - 1e-4f);

    center_depth = ReplaceIfResized(center_depth, pk_ClipParams.y);

    DoFAutoFocusState state = PK_VARIABLE_DATA(pk_DoF_AutoFocusState);
   
    state.focus_depth = lerp_sat(state.focus_depth, center_depth, pk_DeltaTime.x * pk_DoF_FocusSpeed);
    state.focus_depth = max(pk_ClipParams.x, state.focus_depth);

    state.lens_coefficient  = pk_DoF_FocalLength * pk_DoF_FocalLength;
    state.lens_coefficient /= (pk_DoF_FNumber * (state.focus_depth - pk_DoF_FocalLength) * pk_DoF_FilmHeight * 2.0f);

    PK_VARIABLE_DATA(pk_DoF_AutoFocusState) = state;
}