#pragma once
#include "GBuffers.glsl"
#include "SceneGI.glsl"

// Cant combine due to different res (well, can but shouldn't)
uniform image2D pk_GradientInputs;
uniform uimage2DArray pk_Gradients;

#define GRADIENT_STRATA_SIZE 3

struct Gradient { float gradient; uint index; };

float2 Gradient_Load_Input(const int2 coord) { return imageLoad(pk_GradientInputs, coord).xy; }

void Gradient_Store_Input(const int2 coord, float new_input) 
{
    const float2 prev_inputs = Gradient_Load_Input(coord);
    imageStore(pk_GradientInputs, coord, float2(new_input, prev_inputs.x).xyxy); 
} 

Gradient Gradient_Load(const int2 coord, const int layer) 
{
    const uint packed = imageLoad(pk_Gradients, int3(coord, layer)).x;
    return Gradient(uint16BitsToFloat16(ushort(packed & 0xFFFFu)), packed >> 16u);
}

void Gradient_Store(const int2 coord, const int layer, const Gradient g)
{
    const uint packed = uint(float16BitsToUint16(half(g.gradient))) | (g.index << 16u);
    imageStore(pk_Gradients, int3(coord, layer), uint4(packed));
}
