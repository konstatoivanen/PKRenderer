#pragma once
#include <stdio.h>

namespace PK::Utilities::FileIO
{
    int ReadBinary(const char* filepath, void** data, size_t* size);
    int WriteBinary(const char* filepath, void* data, size_t size);
}