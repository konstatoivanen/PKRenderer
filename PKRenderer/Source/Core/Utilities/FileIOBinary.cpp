#include "PrecompiledHeader.h"
#include "FileIOBinary.h"
#include <filesystem>

namespace PK::FileIO
{
    int ReadBinary(const char* filepath, void** data, size_t* size)
    {
        if (strlen(filepath) == 0)
        {
            return -1;
        }

        auto cachepath = std::filesystem::path(std::string(filepath));

        if (!std::filesystem::exists(cachepath))
        {
            return -1;
        }

        FILE* file = nullptr;

#if _WIN32
        auto error = fopen_s(&file, cachepath.string().c_str(), "rb");

        if (error != 0)
        {
            return -1;
        }
#else
        file = fopen(filepath, "rb");
#endif

        if (file == nullptr)
        {
            return -1;
        }

        struct stat filestat;
        int fileNumber = _fileno(file);

        if (fstat(fileNumber, &filestat) != 0)
        {
            fclose(file);
            return -1;
        }

        *size = filestat.st_size;

        if (*size == 0)
        {
            fclose(file);
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

    int ReadBinaryInPlace(const char* filepath, size_t maxSize, void* data, size_t* size)
    {
        if (strlen(filepath) == 0)
        {
            return -1;
        }

        auto cachepath = std::filesystem::path(std::string(filepath));

        if (!std::filesystem::exists(cachepath))
        {
            return -1;
        }

        FILE* file = nullptr;

#if _WIN32
        auto error = fopen_s(&file, cachepath.string().c_str(), "rb");

        if (error != 0)
        {
            return -1;
        }
#else
        file = fopen(filepath, "rb");
#endif

        if (file == nullptr)
        {
            return -1;
        }

        struct stat filestat;
        int fileNumber = _fileno(file);

        if (fstat(fileNumber, &filestat) != 0)
        {
            fclose(file);
            return -1;
        }

        *size = filestat.st_size;

        if (*size == 0 || *size >= maxSize)
        {
            fclose(file);
            return -1;
        }

        fread(data, sizeof(char), *size, file);
        fclose(file);
        return 0;
    }

    int WriteBinary(const char* filepath, void* data, size_t size)
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

#if _WIN32
        auto error = fopen_s(&file, filepath, "wb");

        if (error != 0)
        {
            return -1;
        }
#else
        file = fopen(filepath, "wb");
#endif

        if (file == nullptr)
        {
            return -1;
        }

        fwrite(data, sizeof(char), size, file);
        return fclose(file);
    }
}