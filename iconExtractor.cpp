#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include "iconExtractor.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// This code is a translation of the original JavaScript implementation by gregorygaines.
// The original work can be found at https://github.com/gregorygaines/extract-nds-icons.
// Translated to C++ by marksalamander on 1/22/2025.

namespace fs = std::filesystem;

// Converts 16-bit color palette to RGB values
Icon::Color Icon::getColorFromPalette(const std::vector<uint16_t>& palette, uint8_t index) {
    uint16_t color = palette[index];
    uint8_t r = color & 0x1F;
    uint8_t g = (color >> 5) & 0x1F;
    uint8_t b = (color >> 10) & 0x1F;

    r = (r << 3) + (r >> 2);
    g = (g << 3) + (g >> 2);
    b = (b << 3) + (b >> 2);

    return {r, g, b};
}

// Draws a pixel on the canvas with scaling
void Icon::drawPixel(std::vector<Color>& canvas, int x, int y, const Color& color, int width, int scale) {
    for (int dy = 0; dy < scale; ++dy) {
        for (int dx = 0; dx < scale; ++dx) {
            int offset = ((y * scale + dy) * width + (x * scale + dx));
            canvas[offset] = color;
        }
    }
}

// Saves the canvas image as a png file
void Icon::saveImage(const std::vector<Color>& canvas, int width, int height, std::string filename) {
    fs::path folderPath = fs::current_path() / "icons";
    if (!fs::exists(folderPath)) {
        fs::create_directory(folderPath);
    }
    fs::path fullPath = folderPath / (filename + ".png");
    
    std::vector<uint8_t> image(width * height * 3);
    for (size_t i = 0; i < canvas.size(); ++i) {
        const Color& color = canvas[i];
        image[i * 3] = color.r;
        image[i * 3 + 1] = color.g;
        image[i * 3 + 2] = color.b;
    }

    stbi_write_png(fullPath.string().c_str(), width, height, 3, image.data(), width * 3);
}

// Extracts icon from a .nds ROM file
bool Icon::extractIcon(const std::string& romPath, const std::string fileName, const int scale) {
    std::ifstream rom(romPath, std::ios::binary);
    if (!rom) {
        std::cerr << "Error opening ROM file.\n";
        return false;
    }

    std::uintmax_t romSize = fs::file_size(romPath);
    std::vector<uint8_t> romData(romSize);
    rom.read(reinterpret_cast<char*>(romData.data()), romSize);

    if (!rom) {
        std::cerr << "Error reading ROM file.\n";
        return false;
    }

    // Retrieves tile offset from the ROM data
    uint32_t tileOffset = (romData[iconOffsetAddress + 3] << 24) |
                            (romData[iconOffsetAddress + 2] << 16) |
                            (romData[iconOffsetAddress + 1] << 8) |
                            romData[iconOffsetAddress];

    // Extracts the bitmap from the ROM data starting at offset
    size_t bitmapStartAddress = tileOffset + bitmapOffset;
    std::vector<uint8_t> bitmap(bitmapSize);
    for (size_t i = 0; i < bitmapSize; i++) {
        bitmap[i] = romData[bitmapStartAddress + i];
    }

    // Extracts color palette from the ROM data
    int colorPaletteIndex = 0;
    std::vector<uint16_t> colorPalette(colorPaletteSize);
    const size_t colorPaletteStartAddress = tileOffset + colorPaletteOffset;
    for(int i = 0; i < colorPaletteSize; i++) {
        const int currColorAddress = i*2;

        colorPalette[colorPaletteIndex++] = (romData[(colorPaletteStartAddress + currColorAddress) + 1] << 8) | 
                                            romData[(colorPaletteStartAddress + currColorAddress)];
    }

    const int canvasWidth = 32 * scale;
    const int canvasHeight = 32 * scale;
    std::vector<Color> canvas(canvasWidth * canvasHeight, {255, 255, 255});


    const size_t totalChunks = 16;
    const size_t pixelsPerChunk = 32;

    for(size_t chunk = 0; chunk < totalChunks; chunk++){
        for(size_t pixelIndex =  0; pixelIndex < pixelsPerChunk; pixelIndex++) {
            uint8_t pixel = bitmap[(chunk * pixelsPerChunk) + pixelIndex];
            int x = ((chunk * 8) % 32) + (((pixelIndex * 2) % 8));
            int y = ((std::floor(chunk / 4) * 8) + std::floor(pixelIndex / 4));

            uint8_t leftPixel = pixel & 0xF;
            uint8_t rightPixel = (pixel >> 4) & 0xF;

            if (leftPixel != 0) {
                Color color = getColorFromPalette(colorPalette, leftPixel);
                drawPixel(canvas, x, y, color, canvasWidth, scale);
            }

            if (rightPixel != 0) {
                Color color = getColorFromPalette(colorPalette, rightPixel);
                drawPixel(canvas, x + 1, y, color, canvasWidth, scale);
            }
        }
    }

    saveImage(canvas, canvasWidth, canvasHeight, fileName);
    return true;
}

int main() {
    Icon icon;
    std::string romFile;
    int scale = 32;
    
    bool found = false;
    for(const auto& entry : fs::directory_iterator(fs::current_path())) {
        if(entry.is_regular_file() && entry.path().extension() == ".nds") {
            romFile = entry.path().string();
            std::string fileName = entry.path().stem().string();
            icon.extractIcon(romFile, fileName, scale);
            found = true;
        }
    }

    if(!found) {
        std::cout << "No .nds file found in the current directory.\n";
    }

    return 0;
}