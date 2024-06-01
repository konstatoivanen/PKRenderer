#pragma PROGRAM_COMPUTE
#include includes/PostFXDepthOfField.glsl

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    float centerDepth = min(SampleMinZ(float2(0.5f, 0.5f), 4), pk_ClipParams.y - 1e-4f);
    centerDepth = ReplaceIfResized(centerDepth, pk_ClipParams.y);

    AutoFocusData data = PK_VARIABLE_DATA(pk_DoF_AutoFocusParams);
    data.Distance = lerp_sat(data.Distance, centerDepth, pk_DeltaTime.x * pk_DoF_FocusSpeed);
    data.Distance = max(pk_ClipParams.x, data.Distance);
    data.LensCoefficient = GetLensCoefficient(data.Distance);
    PK_VARIABLE_DATA(pk_DoF_AutoFocusParams) = data;
}