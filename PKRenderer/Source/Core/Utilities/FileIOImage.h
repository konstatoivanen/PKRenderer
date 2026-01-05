#pragma once
#include <cstdint>

namespace PK::FileIO
{
    struct Image
    {
        byte* pixels;
        int32_t width;
        int32_t height;
        int32_t bytesPerPixel;
    };

    Image* ReadBMP(const char* fileName);
    Image* ReadICO(const char* fileName);
    Image* ReadImage(const char* fileName);

    void WriteBMP(const char* fileName, const Image& image);
}
