#ifndef ICON_H
#define ICON_H

#include <vector>
#include <cstdint>
#include <string>

class Icon {
public:
    bool extractIcon(const std::string& romPath, const std::string fileName, const int scale);

private:
    const size_t iconOffsetAddress = 0x68;

    const size_t bitmapSize = 0x200;
    const size_t bitmapOffset = 0x20;

    const size_t colorPaletteOffset = 0x220;
    const size_t colorPaletteSize = 16;
    struct Color {
        uint8_t r, g, b;
    };

    Color getColorFromPalette(const std::vector<uint16_t>& palette, uint8_t index);
    void drawPixel(std::vector<Color>& canvas, int x, int y, const Color& color, int width, int scale);
    void saveImage(const std::vector<Color>& canvas, int width, int height, std::string filename);
};

#endif