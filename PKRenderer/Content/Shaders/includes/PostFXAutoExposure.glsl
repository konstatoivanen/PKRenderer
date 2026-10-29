#pragma once
#include "Utilities.glsl"

uniform RWStructuredBufferRef<uint> pk_AutoExposure_Histogram;

float GetAutoExposure()
{
    return asfloat(pk_AutoExposure_Histogram[256]);
}
