#include "PrecompiledHeader.h"
#include "FileIOBMP.h"

namespace PK::Utilities::FileIO
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


    void ReadBMP(const char* fileName, byte** pixels, int32_t* width, int32_t* height, int32_t* bytesPerPixel)
    {
        FILE* imageFile = fopen(fileName, "rb");
        int32_t dataOffset;
        fseek(imageFile, DATA_OFFSET_OFFSET, SEEK_SET);
        fread(&dataOffset, 4, 1, imageFile);
        fseek(imageFile, WIDTH_OFFSET, SEEK_SET);
        fread(width, 4, 1, imageFile);
        fseek(imageFile, HEIGHT_OFFSET, SEEK_SET);
        fread(height, 4, 1, imageFile);
        int16_t bitsPerPixel;
        fseek(imageFile, BITS_PER_PIXEL_OFFSET, SEEK_SET);
        fread(&bitsPerPixel, 2, 1, imageFile);
        *bytesPerPixel = ((int32_t)bitsPerPixel) / 8;

        int paddedRowSize = (int)(4 * ceil((float)(*width) / 4.0f)) * (*bytesPerPixel);
        int unpaddedRowSize = (*width) * (*bytesPerPixel);
        int totalSize = unpaddedRowSize * (*height);

        *pixels = (byte*)malloc(totalSize);

        byte* currentRowPointer = *pixels + ((*height - 1) * unpaddedRowSize);

        for (auto i = 0; i < *height; ++i)
        {
            fseek(imageFile, dataOffset + (i * paddedRowSize), SEEK_SET);
            fread(currentRowPointer, 1, unpaddedRowSize, imageFile);

            // Flip BGRA to RGBA
            for (auto j = 0; j < *width; ++j)
            {
                byte* pixel = currentRowPointer + j * (*bytesPerPixel);
                byte values[4] = { pixel[2], pixel[1], pixel[0], pixel[3] };
                memcpy(pixel, values, sizeof(values));
            }

            currentRowPointer -= unpaddedRowSize;
        }

        fclose(imageFile);
    }

    void WriteBMP(const char* fileName, byte* pixels, uint32_t width, uint32_t height)
    {
        FILE* outputFile = fopen(fileName, "wb");
        //*****HEADER************//
        const char* BM = "BM";
        fwrite(&BM[0], 1, 1, outputFile);
        fwrite(&BM[1], 1, 1, outputFile);
        int32_t paddedRowSize = (int32_t)(4 * ceil((float)width / 4.0f)) * BYTES_PER_PIXEL;
        uint32_t fileSize = paddedRowSize * height + HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&fileSize, 4, 1, outputFile);
        uint32_t reserved = 0x0000;
        fwrite(&reserved, 4, 1, outputFile);
        uint32_t dataOffset = HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&dataOffset, 4, 1, outputFile);

        //*******INFO*HEADER******//
        uint32_t infoHeaderSize = INFO_HEADER_SIZE;
        fwrite(&infoHeaderSize, 4, 1, outputFile);
        fwrite(&width, 4, 1, outputFile);
        fwrite(&height, 4, 1, outputFile);
        uint16_t planes = 1; //always 1
        fwrite(&planes, 2, 1, outputFile);
        uint16_t bitsPerPixel = BYTES_PER_PIXEL * 8;
        fwrite(&bitsPerPixel, 2, 1, outputFile);
        //write compression
        uint32_t compression = NO_COMPRESION;
        fwrite(&compression, 4, 1, outputFile);
        //write image size(in bytes)
        uint32_t imageSize = width * height * BYTES_PER_PIXEL;
        fwrite(&imageSize, 4, 1, outputFile);
        uint32_t resolutionX = 11811; //300 dpi
        uint32_t resolutionY = 11811; //300 dpi
        fwrite(&resolutionX, 4, 1, outputFile);
        fwrite(&resolutionY, 4, 1, outputFile);
        uint32_t colorsUsed = MAX_NUMBER_OF_COLORS;
        fwrite(&colorsUsed, 4, 1, outputFile);
        uint32_t importantColors = ALL_COLORS_REQUIRED;
        fwrite(&importantColors, 4, 1, outputFile);
        int32_t unpaddedRowSize = width * BYTES_PER_PIXEL;

        for (int32_t y = height - 1; y >= 0; --y)
            for (uint32_t x = 0u; x < width; ++x)
            {
                uint32_t index = (x + y * width) * BYTES_PER_PIXEL;
                byte color[4] = { pixels[index + 0], pixels[index + 1], pixels[index + 2], pixels[index + 3] };
                fwrite(color, sizeof(byte), 4, outputFile);
            }

        fclose(outputFile);
    }
}