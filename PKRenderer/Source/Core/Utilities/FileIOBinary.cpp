#include "PrecompiledHeader.h"
#include "FileIOBinary.h"
#include <filesystem>

namespace PK::FileIO
{
    FILE* OpenFile(const char* filepath, const char* openMode, size_t* outSize)
    {
        if (strlen(filepath) == 0)
        {
            return nullptr;
        }

        auto cachepath = std::filesystem::path(std::string(filepath));

        if (!std::filesystem::exists(cachepath))
        {
            return nullptr;
        }

        FILE* file = fopen(cachepath.string().c_str(), openMode);

        if (file == nullptr)
        {
            return nullptr;
        }

        struct stat filestat;
        int fileNumber = _fileno(file);

        if (fstat(fileNumber, &filestat) != 0)
        {
            fclose(file);
            return nullptr;
        }

        *outSize = filestat.st_size;

        if (*outSize == 0)
        {
            fclose(file);
            return nullptr;
        }

        return file;
    }

    int ReadBinary(const char* filepath, bool isText, void** data, size_t* size)
    {
        FILE* file = OpenFile(filepath, isText ? "r" : "rb", size);

        if (file == nullptr)
        {
            return -1;
        }

        *data = malloc(*size);

        if (*data == nullptr)
        {
            fclose(file);
            return -1;
        }

        fread(*data, sizeof(char), *size, file);
        fclose(file);
        return 0;
    }

    int ReadBinaryInPlace(const char* filepath, bool isText, size_t maxSize, void* data, size_t* size)
    {
        FILE* file = OpenFile(filepath, isText ? "r" : "rb", size);

        if (file == nullptr)
        {
            return -1;
        }

        if (*size >= maxSize)
        {
            fclose(file);
            return -1;
        }

        fread(data, sizeof(char), *size, file);
        fclose(file);
        return 0;
    }

    int WriteBinary(const char* filepath, bool isText, void* data, size_t size)
    {
        FILE* file = nullptr;

        auto path = std::filesystem::path(filepath).remove_filename().string();
        path = path.substr(0, path.length() - 1);

        if (!std::filesystem::exists(path))
        {
            try
            {
                std::filesystem::create_directories(path);
            }
            catch (std::exception& e)
            {
                printf("%s", e.what());
            }
        }

        file = fopen(filepath, isText ? "w" : "wb");

        if (file == nullptr)
        {
            return -1;
        }

        fwrite(data, sizeof(char), size, file);
        return fclose(file);
    }
}