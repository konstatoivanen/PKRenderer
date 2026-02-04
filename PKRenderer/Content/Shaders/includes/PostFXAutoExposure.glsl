#pragma once
#include "Utilities.glsl"

PK_DECLARE_BUFFER(uint, pk_AutoExposure_Histogram);

float GetAutoExposure()
{
    return uintBitsToFloat(pk_AutoExposure_Histogram[256]);
}
