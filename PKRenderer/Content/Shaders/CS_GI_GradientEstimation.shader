#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_GRADIENT_FILTER

#include includes/SceneGIGradients.glsl

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
#if defined(PK_GI_GRADIENT_FILTER)
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const int2 size = int2(imageSize(pk_Gradients).xy);

    if (coord.x >= size.x || coord.y >= size.y)
    {
        return;
    }

    const float waveletKernel[2][2] =
    {
        { 1.0, 0.5  },
        { 0.5, 0.25 }
    };

    Gradient gradient = Gradient_Load(coord, 1);
    gradient.gradient = 0.0f;
    float wSum = 0.0f;

    for (int yy = -1; yy <= 1; ++yy)
        for (int xx = -1; xx <= 1; ++xx)
        {
            const int2 xy = coord + int2(xx, yy);
            const float s_gradient = Gradient_Load(xy, 1).gradient;
            const float s_w = waveletKernel[abs(yy)][abs(xx)] * float(All_InArea(xy, int2(0), size));
            gradient.gradient += s_gradient * s_w;
            wSum += s_w;
        }

    gradient.gradient /= wSum;
    Gradient_Store(coord, 0, gradient);

#else
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const int2 size = int2(imageSize(pk_GradientInputs).xy);

    if (coord.x * GRADIENT_STRATA_SIZE >= size.x || coord.y * GRADIENT_STRATA_SIZE >= size.y)
    {
        return;
    }

    Gradient gradientPre = Gradient_Load(coord, 0);
    Gradient gradientCur = Gradient(0.0f, 0u);

    int2 coordPre = int2(0);
    int2 coordCur = int2(0);
    float2 inputs = 0.0f.xx;

    for (uint yy = 0u; yy < GRADIENT_STRATA_SIZE; ++yy)
        for (uint xx = 0u; xx < GRADIENT_STRATA_SIZE; ++xx)
        {
            if (yy * GRADIENT_STRATA_SIZE + xx == gradientPre.index)
            {
                continue;
            }

            const int2 s_coordCur = coord * GRADIENT_STRATA_SIZE + int2(xx, yy);

            const int2 s_coordFull = GI_ExpandCheckerboardCoord(s_coordCur, 1u);
            const float s_depth = SamplePreviousViewDepth(s_coordFull);
            const float2 s_screenuv = ViewToClipUVPrev(CoordToViewPos(s_coordFull, s_depth)) * int2(pk_ScreenSize.xy);

            const int2 s_coordPre = GI_CollapseCheckerboardCoord(s_screenuv, 1u);
            const float2 s_inputs = Gradient_Load_Input(s_coordPre);

            if (s_inputs.y > inputs.y)
            {
                coordCur = s_coordCur;
                coordPre = s_coordPre;
                inputs = s_inputs;
                gradientCur.index = yy * GRADIENT_STRATA_SIZE + xx;
            }
        }

    const float c_depth = SampleViewDepth(GI_ExpandCheckerboardCoord(coordCur));
    const float p_depth = SamplePreviousViewDepth(GI_ExpandCheckerboardCoord(coordPre, 1u));

    if (Test_DepthReproject(c_depth, p_depth, 0.5f) || Test_DepthFar(c_depth) || inputs.y <= 0.0)
    {
        Gradient_Store(coord, 1, Gradient(0.0f, 0xFFFFu));
        return;
    }

    const float gradSample = inputs.x - inputs.y;
    const float normFactor = max(inputs.x, inputs.y);
    gradientCur.gradient = normFactor > 0.0001f ? saturate(abs(gradSample) / normFactor) : 0.0;
    gradientCur.gradient = mix(gradientCur.gradient, gradientPre.gradient, 0.5f);
    Gradient_Store(coord, 1, gradientCur);
#endif
}
