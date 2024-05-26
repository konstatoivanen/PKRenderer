#pragma once
#include <cstdint>

namespace PK::FileIO
{
    void ReadBMP(const char* fileName, byte** pixels, int32_t* width, int32_t* height, int32_t* bytesPerPixel);
    void WriteBMP(const char* fileName, byte* pixels, uint32_t width, uint32_t height);
}