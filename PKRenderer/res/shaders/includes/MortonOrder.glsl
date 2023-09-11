#pragma once

// Source: https://gist.github.com/franjaviersans/885c136932ef37d8905a6433d0828be6
uint IntegerCompact2(uint x)
{
    x = (x & 0x11111111) | ((x & 0x44444444) >> 1);
    x = (x & 0x03030303) | ((x & 0x30303030) >> 2);
    x = (x & 0x000F000F) | ((x & 0x0F000F00) >> 4);
    x = (x & 0x000000FF) | ((x & 0x00FF0000) >> 8);
    return x;
}

uint IntegerCompact3(uint n)
{
	n &= 0x09249249;
	n = (n ^ (n >> 2)) & 0x030c30c3;
	n = (n ^ (n >> 4)) & 0x0300f00f;
	n = (n ^ (n >> 8)) & 0xff0000ff;
	n = (n ^ (n >> 16)) & 0x000003ff;
	return n;
}

uint IntegerExplode(uint x)
{
    x = (x | (x << 8)) & 0x00FF00FF;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;
    return x;
}

uint ZCurveToIndex2D(uint2 xy) { return IntegerExplode(xy.x) | (IntegerExplode(xy.y) << 1); }
uint2 IndexToZCurve2D(uint x) { return uint2(IntegerCompact2(x), IntegerCompact2(x >> 1u)); }
uint3 IndexToZCurve3D(uint x) { return uint3(IntegerCompact3(x), IntegerCompact3(x >> 1u), IntegerCompact3(x >> 2u)); }