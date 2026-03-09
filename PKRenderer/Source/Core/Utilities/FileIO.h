#pragma once
#include <stdint.h>

namespace PK::FileIO
{
    struct Image
    {
        uint8_t* pixels;
        int32_t width;
        int32_t height;
        int32_t bytesPerPixel;
    };

    bool DirectoryExists(const char* directory);
    bool FileExists(const char* path);

    int ReadBinary(const char* filepath, bool isText, void** data, size_t * size);
    int ReadBinaryInPlace(const char* filepath, bool isText, size_t maxSize, void* data, size_t * size);
    int WriteBinary(const char* filepath, bool isText, void* data, size_t size);

    Image* ReadBMP(const char* fileName);
    Image* ReadICO(const char* fileName);
    Image* ReadImage(const char* fileName);

    void WriteBMP(const char* fileName, const Image& image);
}
