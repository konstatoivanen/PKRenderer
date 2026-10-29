#pragma once
#include "Utilities.glsl"

uniform RWStructuredBuffer<uint> pk_AutoExposure_Histogram;

float GetAutoExposure()
{
    return asfloat(pk_AutoExposure_Histogram[256]);
}
