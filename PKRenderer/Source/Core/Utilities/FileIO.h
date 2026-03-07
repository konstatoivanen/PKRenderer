#pragma once
#include <stdint.h>

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

    int ReadBinary(const char* filepath, bool isText, void** data, size_t * size);
    int ReadBinaryInPlace(const char* filepath, bool isText, size_t maxSize, void* data, size_t * size);
    int WriteBinary(const char* filepath, bool isText, void* data, size_t size);
}
