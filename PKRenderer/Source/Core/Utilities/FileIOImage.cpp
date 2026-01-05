#include "PrecompiledHeader.h"
#include "FileIOImage.h"

namespace PK::FileIO
{
    // Source: https://elcharolin.wordpress.com/2018/11/28/read-and-write-bmp-files-in-c-c/
    constexpr static const uint32_t BYTES_PER_PIXEL = 4u;
    constexpr static const int32_t DATA_OFFSET_OFFSET = 0x000A;
    constexpr static const int32_t WIDTH_OFFSET = 0x0012;
    constexpr static const int32_t HEIGHT_OFFSET = 0x0016;
    constexpr static const int32_t BITS_PER_PIXEL_OFFSET = 0x001C;
    constexpr static const int32_t HEADER_SIZE = 14;
    constexpr static const int32_t INFO_HEADER_SIZE = 40;
    constexpr static const int32_t NO_COMPRESION = 0;
    constexpr static const int32_t MAX_NUMBER_OF_COLORS = 0;
    constexpr static const int32_t ALL_COLORS_REQUIRED = 0;

    Image* ReadBMP(const char* fileName)
    {
        if (fileName == nullptr)
        {
            return nullptr;
        }

        FILE* file = fopen(fileName, "rb");
        int32_t dataOffset;
        int16_t bitsPerPixel;
        int32_t width;
        int32_t height;

        char header[2];
        fread(header, sizeof(char), 2, file);

        if (header[0] != 'B' || header[1] != 'M')
        {
            return nullptr;
        }

        fseek(file, DATA_OFFSET_OFFSET, SEEK_SET);
        fread(&dataOffset, 4, 1, file);
        fseek(file, WIDTH_OFFSET, SEEK_SET);
        fread(&width, 4, 1, file);
        fseek(file, HEIGHT_OFFSET, SEEK_SET);
        fread(&height, 4, 1, file);
        fseek(file, BITS_PER_PIXEL_OFFSET, SEEK_SET);
        fread(&bitsPerPixel, 2, 1, file);

        auto bytesPerPixel = ((int32_t)bitsPerPixel) / 8;
        auto paddedRowSize = (int)(4 * ceil((float)width / 4.0f)) * bytesPerPixel;
        auto unpaddedRowSize = width * bytesPerPixel;
        auto totalSize = unpaddedRowSize * height;

        auto buffer = reinterpret_cast<char*>(malloc(sizeof(Image) + totalSize));
        auto image = reinterpret_cast<Image*>(buffer);
        image->pixels = reinterpret_cast<byte*>(buffer + sizeof(Image));
        image->width = width;
        image->height = height / 2;
        image->bytesPerPixel = bytesPerPixel;

        byte* currentRowPointer = image->pixels + ((height - 1) * unpaddedRowSize);

        for (auto i = 0; i < height; ++i)
        {
            fseek(file, dataOffset + (i * paddedRowSize), SEEK_SET);
            fread(currentRowPointer, 1, unpaddedRowSize, file);

            // Flip BGRA to RGBA
            for (auto j = 0; j < width; ++j)
            {
                byte* pixel = currentRowPointer + j * (bytesPerPixel);
                byte values[4] = { pixel[2], pixel[1], pixel[0], pixel[3] };
                memcpy(pixel, values, sizeof(values));
            }

            currentRowPointer -= unpaddedRowSize;
        }

        fclose(file);
        return image;
    }

    Image* ReadICO(const char* fileName)
    {
        if (fileName == nullptr)
        {
            return nullptr;
        }

        FILE* file = fopen(fileName, "rb");
        
        uint16_t reserved;
        uint16_t imageType;
        uint16_t imageCount;
        fread(&reserved, sizeof(uint16_t), 1, file);
        fread(&imageType, sizeof(uint16_t), 1, file);
        fread(&imageCount, sizeof(uint16_t), 1, file);

        if (imageCount == 0u || imageType != 1)
        {
            fclose(file);
            return nullptr;
        }

        // Largest image is stored first.
        uint32_t offset;
        fseek(file, 12, SEEK_CUR);
        fread(&offset, sizeof(uint32_t), 1, file);
        fseek(file, offset, SEEK_SET);

        // Read as bmp... if it's a png then we're in trouble.
        uint32_t biSize;
        int32_t biWidth;
        int32_t biHeight;
        uint16_t biPlanes;
        uint16_t biBitCount;
        uint32_t biCompression;
        uint32_t biSizeImage;
        int32_t biXPelsPerMeter;
        int32_t biYPelsPerMeter;
        uint32_t biClrUsed;
        uint32_t biClrImportant;
        fread(&biSize, sizeof(uint32_t), 1, file);
        fread(&biWidth, sizeof(int32_t), 1, file);
        fread(&biHeight, sizeof(int32_t), 1, file);
        fread(&biPlanes, sizeof(uint16_t), 1, file);
        fread(&biBitCount, sizeof(uint16_t), 1, file);
        fread(&biCompression, sizeof(uint32_t), 1, file);
        fread(&biSizeImage, sizeof(uint32_t), 1, file);
        fread(&biXPelsPerMeter, sizeof(int32_t), 1, file);
        fread(&biYPelsPerMeter, sizeof(int32_t), 1, file);
        fread(&biClrUsed, sizeof(uint32_t), 1, file);
        fread(&biClrImportant, sizeof(uint32_t), 1, file);

        // Only 4 channel true color are images supported.
        if (biBitCount != 32)
        {
            fclose(file);
            return nullptr;
        }

        biHeight /= 2;
        auto dataOffset = offset + biSize + biClrUsed * 4u;
        auto bytesPerPixel = ((int32_t)biBitCount) / 8;
        auto paddedRowSize = (int)(4 * ceil((float)biWidth / 4.0f)) * bytesPerPixel;
        auto unpaddedRowSize = biWidth * bytesPerPixel;
        auto totalSize = unpaddedRowSize * biHeight;

        auto buffer = reinterpret_cast<char*>(malloc(sizeof(Image) + totalSize));
        auto image = reinterpret_cast<Image*>(buffer);
        image->pixels = reinterpret_cast<byte*>(buffer + sizeof(Image));
        image->width = biWidth;
        image->height = biHeight;
        image->bytesPerPixel = bytesPerPixel;

        byte* currentRowPointer = image->pixels + ((image->height - 1) * unpaddedRowSize);

        for (auto i = 0; i < image->height; ++i)
        {
            fseek(file, dataOffset + (i * paddedRowSize), SEEK_SET);
            fread(currentRowPointer, 1, unpaddedRowSize, file);

            // Flip BGRA to RGBA
            for (auto j = 0; j < image->width; ++j)
            {
                byte* pixel = currentRowPointer + j * 4u;
                byte values[4] = { pixel[2], pixel[1], pixel[0], pixel[3] };
                memcpy(pixel, values, sizeof(values));
            }

            currentRowPointer -= unpaddedRowSize;
        }

        fclose(file);
        return image;
    }

    Image* ReadImage(const char* fileName)
    {
        if (fileName == nullptr)
        {
            return nullptr;
        }

        auto pos = strchr(fileName, '.');

        if (pos == nullptr)
        {
            return nullptr;
        }

        if (strcmp(pos, ".bmp") == 0)
        {
            return ReadBMP(fileName);
        }

        if (strcmp(pos, ".ico") == 0)
        {
            return ReadICO(fileName);
        }

        return nullptr;
    }

    void WriteBMP(const char* fileName, const Image& image)
    {
        FILE* outputFile = fopen(fileName, "wb");
        //*****HEADER************//
        const char* BM = "BM";
        fwrite(&BM[0], 1, 1, outputFile);
        fwrite(&BM[1], 1, 1, outputFile);
        int32_t paddedRowSize = (int32_t)(4 * ceil((float)image.width / 4.0f)) * BYTES_PER_PIXEL;
        uint32_t fileSize = paddedRowSize * image.height + HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&fileSize, 4, 1, outputFile);
        uint32_t reserved = 0x0000;
        fwrite(&reserved, 4, 1, outputFile);
        uint32_t dataOffset = HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&dataOffset, 4, 1, outputFile);

        //*******INFO*HEADER******//
        uint32_t infoHeaderSize = INFO_HEADER_SIZE;
        fwrite(&infoHeaderSize, 4, 1, outputFile);
        fwrite(&image.width, 4, 1, outputFile);
        fwrite(&image.height, 4, 1, outputFile);
        uint16_t planes = 1; //always 1
        fwrite(&planes, 2, 1, outputFile);
        uint16_t bitsPerPixel = BYTES_PER_PIXEL * 8;
        fwrite(&bitsPerPixel, 2, 1, outputFile);
        //write compression
        uint32_t compression = NO_COMPRESION;
        fwrite(&compression, 4, 1, outputFile);
        //write image size(in bytes)
        uint32_t imageSize = image.width * image.height * BYTES_PER_PIXEL;
        fwrite(&imageSize, 4, 1, outputFile);
        uint32_t resolutionX = 11811; //300 dpi
        uint32_t resolutionY = 11811; //300 dpi
        fwrite(&resolutionX, 4, 1, outputFile);
        fwrite(&resolutionY, 4, 1, outputFile);
        uint32_t colorsUsed = MAX_NUMBER_OF_COLORS;
        fwrite(&colorsUsed, 4, 1, outputFile);
        uint32_t importantColors = ALL_COLORS_REQUIRED;
        fwrite(&importantColors, 4, 1, outputFile);

        for (int32_t y = image.height - 1; y >= 0; --y)
        for (int32_t x = 0; x < image.width; ++x)
        {
            uint32_t index = (x + y * image.width) * BYTES_PER_PIXEL;
            byte color[4] = { image.pixels[index + 0], image.pixels[index + 1], image.pixels[index + 2], image.pixels[index + 3] };
            fwrite(color, sizeof(byte), 4, outputFile);
        }

        fclose(outputFile);
    }
}
