#include <stdio.h>
#include <stdint.h>

// Define BMP file headers
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint32_t reserved1;
    uint32_t offset;
} BmpFileHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t imagesize;
    int32_t xresolution;
    int32_t yresolution;
    uint32_t ncolors;
    uint32_t importantcolors;
} BmpInfoHeader;
#pragma pack(pop)

// Create a 2D array with your pixel values (0-255)
// Example: int pixels[width][height];

void createBitmap(const char* filename, int width, int height, int pixels[width][height]) {
    BmpFileHeader fileHeader = { 0x4D42, sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + width * height * 3, 0, sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) };
    BmpInfoHeader infoHeader = { sizeof(BmpInfoHeader), width, height, 1, 24, 0, width * height * 3, 0, 0, 0, 0 };

    FILE* bmpFile = fopen(filename, "wb");
    if (!bmpFile) {
        perror("Error opening file");
        return;
    }

    // Write headers
    fwrite(&fileHeader, sizeof(BmpFileHeader), 1, bmpFile);
    fwrite(&infoHeader, sizeof(BmpInfoHeader), 1, bmpFile);

    // Write pixel data (BGR format)
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            uint8_t b = pixels[x][y] & 0xFF;
            uint8_t g = pixels[x][y] & 0xFF;
            uint8_t r = pixels[x][y] & 0xFF;
            fwrite(&b, 1, 1, bmpFile);
            fwrite(&g, 1, 1, bmpFile);
            fwrite(&r, 1, 1, bmpFile);
        }
    }

    fclose(bmpFile);
}

int main() {
    // Example usage
    const int width = 256;
    const int height = 256;
    int pixels[width][height]; // Fill this array with your pixel values

    createBitmap("mandeloutput.bmp", width, height, pixels);
    printf("Bitmap file created successfully!\n");

    return 0;
}
