#pragma once
#include <stdio.h>

namespace PK::FileIO
{
    int ReadBinary(const char* filepath, bool isText, void** data, size_t* size);
    int ReadBinaryInPlace(const char* filepath, bool isText, size_t maxSize, void* data, size_t* size);
    int WriteBinary(const char* filepath, bool isText, void* data, size_t size);
}
