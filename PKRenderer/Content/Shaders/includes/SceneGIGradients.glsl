#pragma once
#include "GBuffers.glsl"
#include "SceneGI.glsl"

// Cant combine due to different res (well, can but shouldn't)
uniform RWTexture2DRef<float2> pk_GradientInputs;
uniform RWTexture2DArrayRef<uint> pk_Gradients;

#define GRADIENT_STRATA_SIZE 3

struct Gradient { float gradient; uint index; };

float2 Gradient_Load_Input(const int2 coord) { return pk_GradientInputs[coord].xy; }

void Gradient_Store_Input(const int2 coord, float new_input) 
{
    const float2 prev_inputs = Gradient_Load_Input(coord);
    pk_GradientInputs[coord] = float2(new_input, prev_inputs.x); 
} 

Gradient Gradient_Load(const int2 coord, const int layer) 
{
    const uint packed = pk_Gradients[int3(coord, layer)];
    return Gradient(f16tof32(packed & 0xFFFFu), packed >> 16u);
}

void Gradient_Store(const int2 coord, const int layer, const Gradient g)
{
    const uint packed = (f32tof16(g.gradient) & 0xFFFFu) | (g.index << 16u);
    pk_Gradients[int3(coord, layer)] = packed;
}
