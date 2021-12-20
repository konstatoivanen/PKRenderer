#version 460
#pragma PROGRAM_COMPUTE
#include includes/SharedDepthOfField.glsl

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    float centerDistance = GetTargetDistance();
    AutoFocusData data = PK_ATOMIC_DATA(pk_AutoFocusParams);
    data.Distance = lerp(data.Distance, centerDistance, pk_DeltaTime.x * pk_FocusSpeed);
    data.Distance = max(pk_ProjectionParams.x, data.Distance);
    data.LensCoefficient = GetLensCoefficient(data.Distance);
    PK_ATOMIC_DATA(pk_AutoFocusParams) = data;
}