#pragma once
#include "Utilities.glsl"

PK_DECLARE_BUFFER(uint, pk_AutoExposure_Histogram, PK_SET_PASS);

float GetAutoExposure()
{
    return uintBitsToFloat(PK_BUFFER_DATA(pk_AutoExposure_Histogram, 256));
}
