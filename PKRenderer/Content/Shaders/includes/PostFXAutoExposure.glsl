#pragma once
#include "Utilities.glsl"

uniform RWBuffer<uint> pk_AutoExposure_Histogram;

float GetAutoExposure()
{
    return uintBitsToFloat(pk_AutoExposure_Histogram[256]);
}
