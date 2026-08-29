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

    void FindFiles(void* ctx, const char* directory, const char* pattern, bool recursive, void (*onFile)(void*, const char*));
    bool CreateDirectory(const char* filepath);
    bool DirectoryExists(const char* filepath);
    bool FileExists(const char* filepath);

    int CloseFile(void* file);
    void* OpenWrite(const char* filepath, bool isText);
    void* OpenRead(const char* filepath, bool isText, size_t* outSize = nullptr);

    int Read(const char* filepath, bool isText, void** data, size_t * size);
    int ReadInPlace(const char* filepath, bool isText, size_t maxSize, void* data, size_t * size);
    int Write(const char* filepath, bool isText, void* data, size_t size);

    Image* ReadBMP(const char* filepath);
    Image* ReadICO(const char* filepath);
    Image* ReadImage(const char* filepath);

    int WriteBMP(const char* filepath, const Image& image);
}
