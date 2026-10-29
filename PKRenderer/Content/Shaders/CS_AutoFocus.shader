
#pragma pk_program Compute AutoFocusCs

#include "includes/PostFXDepthOfField.glsl"

uniform float pk_DoF_FocalLength;
uniform float pk_DoF_FNumber;
uniform float pk_DoF_FilmHeight;
uniform float pk_DoF_FocusSpeed;

[numthreads(1u, 1u, 1u)]
void AutoFocusCs()
{
    float center_depth = min(SampleMinZ(0.5f.xx, 4.0f), pk_View.ClipParams.y - 1e-4f);
    center_depth = ReplaceIfResized(center_depth, pk_View.ClipParams.y);

    DoFAutoFocusState state = pk_DoF_AutoFocusState[0];
    state.focus_depth = lerp_sat(state.focus_depth, center_depth, 1.0f - exp(-pk_View.DeltaTime.x * pk_DoF_FocusSpeed));
    state.focus_depth = max(pk_View.ClipParams.x, state.focus_depth);

    state.lens_coefficient  = pk_DoF_FocalLength * pk_DoF_FocalLength;
    state.lens_coefficient /= (pk_DoF_FNumber * (state.focus_depth - pk_DoF_FocalLength) * pk_DoF_FilmHeight * 2.0f);
    pk_DoF_AutoFocusState[0] = state;
}
