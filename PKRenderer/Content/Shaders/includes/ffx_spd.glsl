
#extension GL_KHR_shader_subgroup_quad : require

//_____________________________________________________________/\_______________________________________________________________
//==============================================================================================================================
//
//                                         [FFX SPD] Single Pass Downsampler 2.0
//
//==============================================================================================================================
// LICENSE
// =======
// Copyright (c) 2017-2020 Advanced Micro Devices, Inc. All rights reserved.
// -------
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// -------
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
// -------
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//------------------------------------------------------------------------------------------------------------------------------

/*
Modified from original ffx_spd.h to do the following:
- Remove slice/cube support as they're not needed in this project.
- Remove descriptor indexing.
- Remove dependency on float4 as register packing for float2 should be ok.
- Remove dependency on ffx_a.h

User needs to define the following:

// value type used for the image1D
SpdFormat 

// reduction operator for four value types
SpdFormat SpdReduce4(SpdFormat v0, SpdFormat v1, SpdFormat v2, SpdFormat v3)

// reduction operator for two value types. for non pot reduction
SpdFormat SpdReduce2(SpdFormat v0, SpdFormat v1)

// LDS load store functions 
SpdFormat SpdLoadIntermediate(uint x, uint y)
void SpdStoreIntermediate(uint x, uint y, SpdFormat value)

// atomic counter increment. used to track last workgroup that does writing so that reads from mip 5 are coherent.
void SpdIncreaseAtomicCounter()
uint SpdGetAtomicCounter()

// Load reduction operator to access source texture data for 4 texels.
SpdFormat SpdReduceLoadSourceImage4(int2 base)

// Load operator for last workgroup to get coherent mip6 data
SpdFormat SpdLoadMip6(int2 coord)

// Store operators for all possible mips. main difference from base SPD as we want to have static indexing to the resources.
void SpdStoreMip1(int2 coord, SpdFormat v) 
void SpdStoreMip2(int2 coord, SpdFormat v)
void SpdStoreMip3(int2 coord, SpdFormat v)
void SpdStoreMip4(int2 coord, SpdFormat v)
void SpdStoreMip5(int2 coord, SpdFormat v)
void SpdStoreMip6(int2 coord, SpdFormat v)
void SpdStoreMip7(int2 coord, SpdFormat v)
void SpdStoreMip8(int2 coord, SpdFormat v)
void SpdStoreMip9(int2 coord, SpdFormat v)
void SpdStoreMip10(int2 coord, SpdFormat v)
void SpdStoreMip11(int2 coord, SpdFormat v)
void SpdStoreMip12(int2 coord, SpdFormat v)
*/

SpdFormat SpdReduceQuad(SpdFormat v)
{
    SpdFormat v0 = v;
    SpdFormat v1 = subgroupQuadSwapHorizontal(v);
    SpdFormat v2 = subgroupQuadSwapVertical(v);
    SpdFormat v3 = subgroupQuadSwapDiagonal(v);
    return SpdReduce4(v0, v1, v2, v3);
}

SpdFormat SpdReduceIntermediateBorder(SpdFormat v, uint2 i0, uint2 i1)
{
    SpdFormat v0 = SpdLoadIntermediate(i0.x, i0.y);
    SpdFormat v1 = SpdLoadIntermediate(i1.x, i1.y);
    return SpdReduce2(SpdReduce2(v0, v1), v);
}

SpdFormat SpdReduceLoad4(uint2 base, int2 size)
{
    #if defined(SPD_SUPPORT_NON_POT_6_TO_12)
    // Need to clamp these so that the border handling of lower mips doesnt sample garbage.
    SpdFormat v0 = SpdLoadMip6(min(size - 1, int2(base + uint2(0, 0))));
    SpdFormat v1 = SpdLoadMip6(min(size - 1, int2(base + uint2(0, 1))));
    SpdFormat v2 = SpdLoadMip6(min(size - 1, int2(base + uint2(1, 0))));
    SpdFormat v3 = SpdLoadMip6(min(size - 1, int2(base + uint2(1, 1))));
    SpdFormat v = SpdReduce4(v0, v1, v2, v3);

    const bool is_border_x = bool(size.x & 0x1) && base.x == size.x - 3;
    const bool is_border_y = bool(size.y & 0x1) && base.y == size.y - 3;

    if (is_border_x)
    {
        SpdFormat v4 = SpdLoadMip6(int2(base + uint2(0, 2)));
        SpdFormat v5 = SpdLoadMip6(int2(base + uint2(1, 2)));
        v = SpdReduce2(SpdReduce2(v4, v5), v);
    }

    if (is_border_y)
    {
        SpdFormat v4 = SpdLoadMip6(int2(base + uint2(2, 0)));
        SpdFormat v5 = SpdLoadMip6(int2(base + uint2(2, 1)));
        v = SpdReduce2(SpdReduce2(v4, v5), v);
    }

    if (is_border_x && is_border_y)
    {
        v = SpdReduce2(SpdLoadMip6(int2(base + uint2(2, 2))), v);
    }

    return v;
    #else
    SpdFormat v0 = SpdLoadMip6(int2(base + uint2(0, 0)));
    SpdFormat v1 = SpdLoadMip6(int2(base + uint2(0, 1)));
    SpdFormat v2 = SpdLoadMip6(int2(base + uint2(1, 0)));
    SpdFormat v3 = SpdLoadMip6(int2(base + uint2(1, 1)));
    return SpdReduce4(v0, v1, v2, v3);
    #endif
}

uint ABfiM(uint src,uint ins,uint bits){return bitfieldInsert(src,ins,0,int(bits));}
uint ABfe(uint src,uint off,uint bits){return bitfieldExtract(src,int(off),int(bits));}
uint2 ARmpRed8x8(uint a){return uint2(ABfiM(ABfe(a,2u,3u),a,1u),ABfiM(ABfe(a,3u,3u),ABfe(a,1u,2u),2u));}

void SpdDownsample(uint2 workGroupID, uint localInvocationIndex, uint mips, uint numWorkGroups, uint2 resolution) 
{
    uint2 sub_xy = ARmpRed8x8(localInvocationIndex % 64);
    uint x = sub_xy.x + 8 * ((localInvocationIndex >> 6) % 2);
    uint y = sub_xy.y + 8 * ((localInvocationIndex >> 7));

    // Mip 1-2
    {
        SpdFormat v[4];
        int2 tex = int2(workGroupID.xy * 64) + int2(x * 2, y * 2);
        int2 pix = int2(workGroupID.xy * 32) + int2(x, y);
        v[0] = SpdReduceLoadSourceImage4(tex);
        SpdStoreMip1(pix, v[0]);

        tex = int2(workGroupID.xy * 64) + int2(x * 2 + 32, y * 2);
        pix = int2(workGroupID.xy * 32) + int2(x + 16, y);
        v[1] = SpdReduceLoadSourceImage4(tex);
        SpdStoreMip1(pix, v[1]);
        
        tex = int2(workGroupID.xy * 64) + int2(x * 2, y * 2 + 32);
        pix = int2(workGroupID.xy * 32) + int2(x, y + 16);
        v[2] = SpdReduceLoadSourceImage4(tex);
        SpdStoreMip1(pix, v[2]);
        
        tex = int2(workGroupID.xy * 64) + int2(x * 2 + 32, y * 2 + 32);
        pix = int2(workGroupID.xy * 32) + int2(x + 16, y + 16);
        v[3] = SpdReduceLoadSourceImage4(tex);
        SpdStoreMip1(pix, v[3]);

        v[0] = SpdReduceQuad(v[0]);
        v[1] = SpdReduceQuad(v[1]);
        v[2] = SpdReduceQuad(v[2]);
        v[3] = SpdReduceQuad(v[3]);

        if ((localInvocationIndex % 4) == 0)
        {
            SpdStoreMip2(int2(workGroupID.xy * 16) + int2(x/2, y/2), v[0]);
            SpdStoreIntermediate(x/2, y/2, v[0]);
            SpdStoreMip2(int2(workGroupID.xy * 16) + int2(x/2 + 8, y/2), v[1]);
            SpdStoreIntermediate(x/2 + 8, y/2, v[1]);
            SpdStoreMip2(int2(workGroupID.xy * 16) + int2(x/2, y/2 + 8), v[2]);
            SpdStoreIntermediate(x/2, y/2 + 8, v[2]);
            SpdStoreMip2(int2(workGroupID.xy * 16) + int2(x/2 + 8, y/2 + 8), v[3]);
            SpdStoreIntermediate(x/2 + 8, y/2 + 8, v[3]);
        }
    }

    // Mip 3
    {
        barrier();
        
        SpdFormat v = SpdLoadIntermediate(x, y);
        v = SpdReduceQuad(v);
        // quad index 0 stores result
        if (localInvocationIndex % 4 == 0)
        {
            SpdStoreMip3(int2(workGroupID.xy * 8) + int2(x/2, y/2), v);
            SpdStoreIntermediate(x + (y/2) % 2, y, v);
        }
    }

    // Mip 4
    {
        barrier();

        if (localInvocationIndex < 64)
        {
            SpdFormat v = SpdLoadIntermediate(x * 2 + y % 2,y * 2);
            v = SpdReduceQuad(v);
            // quad index 0 stores result
            if (localInvocationIndex % 4 == 0)
            {   
                SpdStoreMip4(int2(workGroupID.xy * 4) + int2(x/2, y/2), v);
                SpdStoreIntermediate(x * 2 + y/2, y * 2, v);
            }
        }
    }

    // Mip 5
    {
        barrier();

        if (localInvocationIndex < 16)
        {
            SpdFormat v = SpdLoadIntermediate(x * 4 + y,y * 4);
            v = SpdReduceQuad(v);
            // quad index 0 stores result
            if (localInvocationIndex % 4 == 0)
            {   
                SpdStoreMip5(int2(workGroupID.xy * 2) + int2(x/2, y/2), v);
                SpdStoreIntermediate(x / 2 + y, 0, v);
            }
        }
    }

    // Mip 6
    {
        barrier();
       
        if (localInvocationIndex < 4)
        {
            SpdFormat v = SpdLoadIntermediate(localInvocationIndex,0);
            v = SpdReduceQuad(v);
            // quad index 0 stores result
            if (localInvocationIndex % 4 == 0)
            {   
                SpdStoreMip6(int2(workGroupID.xy), v);
            }
        }
    }

    if (mips <= 7)
    {
        return;
    }

    if (localInvocationIndex == 0)
    {
        SpdIncreaseAtomicCounter();
    }

    barrier();

    // Only one work group needed for the last mips.
    if (SpdGetAtomicCounter() != (numWorkGroups - 1))
    {
        return;
    }

    // Mips 7-8
    {
        const int2 size_6 = int2(resolution >> 6u);

        int2 tex = int2(x * 4 + 0, y * 4 + 0);
        int2 pix = int2(x * 2 + 0, y * 2 + 0);
        SpdFormat v0 = SpdReduceLoad4(tex, size_6);
        SpdStoreMip7(pix, v0);

        tex = int2(x * 4 + 2, y * 4 + 0);
        pix = int2(x * 2 + 1, y * 2 + 0);
        SpdFormat v1 = SpdReduceLoad4(tex, size_6);
        SpdStoreMip7(pix, v1);

        tex = int2(x * 4 + 0, y * 4 + 2);
        pix = int2(x * 2 + 0, y * 2 + 1);
        SpdFormat v2 = SpdReduceLoad4(tex, size_6);
        SpdStoreMip7(pix, v2);

        tex = int2(x * 4 + 2, y * 4 + 2);
        pix = int2(x * 2 + 1, y * 2 + 1);
        SpdFormat v3 = SpdReduceLoad4(tex, size_6);
        SpdStoreMip7(pix, v3);
        
        if (mips <= 8)
        {
            return;
        }

        SpdFormat v = SpdReduce4(v0, v1, v2, v3);
        SpdStoreIntermediate(x, y, v);

        // No access to intermediate borders at this stage. need to sync so that we can access borders without texture loads.
        // Explicit odd border check required so that we dont sample outside of lds memory.
        #if defined(SPD_SUPPORT_NON_POT_6_TO_12)
        barrier();
        const int2 size_7 = int2(resolution >> 7u);
        const bool is_border_x = bool(size_7.x & 0x1) && (x * 2) == size_7.x - 3;
        const bool is_border_y = bool(size_7.y & 0x1) && (y * 2) == size_7.y - 3;
        if (is_border_x) { v = SpdReduce2(v, SpdLoadIntermediate(x + 1u, y)); }
        if (is_border_y) { v = SpdReduce2(v, SpdLoadIntermediate(x, y + 1u)); }
        if (is_border_x && is_border_y) { v = SpdReduce2(v, SpdLoadIntermediate(x + 1u, y + 1u)); }
        SpdStoreIntermediate(x, y, v);
        #endif

        SpdStoreMip8(int2(x, y), v);
    }

    // Mip 9
    if (mips > 9)
    {
        barrier();
        
        SpdFormat v = SpdLoadIntermediate(x, y);
        v = SpdReduceQuad(v);
        
        #if defined(SPD_SUPPORT_NON_POT_6_TO_12)
        const int2 size_9 = int2(resolution >> 9u);
        const bool is_border_x = x / 2 == size_9.x - 1;
        const bool is_border_y = y / 2 == size_9.y - 1;
        if (is_border_x) { v = SpdReduceIntermediateBorder(v, uint2(x + 2, y + 0), uint2(x + 2, y + 1)); }
        if (is_border_y) { v = SpdReduceIntermediateBorder(v, uint2(x + 0, y + 2), uint2(x + 1, y + 2)); }
        if (is_border_x && is_border_y) { v = SpdReduce2(v, SpdLoadIntermediate(x + 2u, y + 2u)); }
        #endif

        if (localInvocationIndex % 4 == 0)
        {
            SpdStoreMip9(int2(x/2, y/2), v);
            SpdStoreIntermediate(x + (y/2) % 2, y, v);
        }
    }

    // Mip 10
    if (mips > 10)
    {
        barrier();

        if (localInvocationIndex < 64)
        {
            SpdFormat v = SpdLoadIntermediate(x * 2 + y % 2,y * 2);
            v = SpdReduceQuad(v);

            #if defined(SPD_SUPPORT_NON_POT_6_TO_12)
            const int2 size_10 = int2(resolution >> 10u);
            const bool is_border_x = (x / 2) == size_10.x - 1;
            const bool is_border_y = (y / 2) == size_10.y - 1;
            if (is_border_x) { v = SpdReduceIntermediateBorder(v, uint2((x + 2) * 2 + (y + 0) % 2, (y + 0) * 2), uint2((x + 2) * 2 + (y + 1) % 2, (y + 1) * 2)); }
            if (is_border_y) { v = SpdReduceIntermediateBorder(v, uint2((x + 0) * 2 + (y + 2) % 2, (y + 2) * 2), uint2((x + 1) * 2 + (y + 2) % 2, (y + 2) * 2)); }
            if (is_border_x && is_border_y) { v = SpdReduce2(v, SpdLoadIntermediate((x + 2) * 2 + y % 2, (y + 2) * 2)); }
            #endif

            // quad index 0 stores result
            if (localInvocationIndex % 4 == 0)
            {   
                SpdStoreMip10(int2(x/2, y/2), v);
                SpdStoreIntermediate(x * 2 + y/2, y * 2, v);
            }
        }
    }

    // Mip 11
    if (mips > 11)
    {
        barrier();

        if (localInvocationIndex < 16)
        {
            SpdFormat v = SpdLoadIntermediate(x * 4 + y, y * 4);
            v = SpdReduceQuad(v);
         
            #if defined(SPD_SUPPORT_NON_POT_6_TO_12)
            const int2 size_11 = int2(resolution >> 11u);
            const bool is_border_x = (x / 2) == size_11.x - 1;
            const bool is_border_y = (y / 2) == size_11.y - 1;
            if (is_border_x) { v = SpdReduceIntermediateBorder(v, uint2((x + 2) * 4 + (y + 0), (y + 0) * 4), uint2((x + 2) * 4 + (y + 1), (y + 1) * 4)); }
            if (is_border_y) { v = SpdReduceIntermediateBorder(v, uint2((x + 0) * 4 + (y + 2), (y + 2) * 4), uint2((x + 1) * 4 + (y + 2), (y + 2) * 4)); }
            if (is_border_x && is_border_y) { v = SpdReduce2(v, SpdLoadIntermediate((x + 2) * 4 + (y + 2), (y + 2) * 4)); }
            #endif
         
            // quad index 0 stores result
            if (localInvocationIndex % 4 == 0)
            {   
                SpdStoreMip11(int2(x/2, y/2), v);
                SpdStoreIntermediate(x / 2 + y, 0, v);
            }
        }
    }

    // Mip 12
    if (mips > 12)
    {
        barrier();
       
        if (localInvocationIndex < 4)
        {
            SpdFormat v = SpdLoadIntermediate(localInvocationIndex,0);
            v = SpdReduceQuad(v);
            // quad index 0 stores result
            if (localInvocationIndex % 4 == 0)
            {   
                SpdStoreMip12(int2(0,0), v);
            }
        }
    }
}
