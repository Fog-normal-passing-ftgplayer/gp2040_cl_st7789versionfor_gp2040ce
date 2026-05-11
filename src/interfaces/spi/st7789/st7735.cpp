#include "st7735.h"
#include "storagemanager.h"
#include "config.pb.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

void GPGFX_ST7735::runTestPattern() {
    if (frameBuffer == nullptr) return;
    const uint16_t colors[] = { 0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000 };
    for (size_t c = 0; c < sizeof(colors)/sizeof(colors[0]); c++) {
        for (size_t i = 0; i < (size_t)frameWidth * frameHeight; i++) {
            frameBuffer[i] = colors[c];
        }
        drawBuffer(nullptr);
        sleep_ms(400);
    }
}

#ifndef DISPLAY_SPI_SPEED
#define DISPLAY_SPI_SPEED 10000000
#endif

GPGFX_ST7735::~GPGFX_ST7735() {
    if (frameBuffer != nullptr) {
        delete[] frameBuffer;
        frameBuffer = nullptr;
    }
}

void GPGFX_ST7735::init(GPGFX_DisplayTypeOptions options) {
    _options = options;

    if (options.spi == nullptr) return;

    // Read SPI display pins from DisplayOptions (set by SPIDisplayConfig web page)
    const DisplayOptions& dispOpts = Storage::getInstance().getDisplayOptions();
    _pinDC  = (dispOpts.spiPinDc  != -1) ? (uint8_t)dispOpts.spiPinDc  : (uint8_t)-1;
    _pinCS  = (dispOpts.spiPinCs  != -1) ? (uint8_t)dispOpts.spiPinCs  : (uint8_t)-1;
    _pinRST = (dispOpts.spiPinRst != -1) ? (uint8_t)dispOpts.spiPinRst : (uint8_t)-1;
    _pinBL  = (dispOpts.spiPinBl  != -1) ? (int8_t)dispOpts.spiPinBl   : (int8_t)-1;

    // Read SPI pins from DisplayOptions (set by SPIDisplayConfig web page)
    uint8_t sck = (dispOpts.spiPinSck != -1) ? (uint8_t)dispOpts.spiPinSck : (uint8_t)-1;
    uint8_t tx  = (dispOpts.spiPinTx  != -1) ? (uint8_t)dispOpts.spiPinTx  : (uint8_t)-1;

    // Read column/row offset (for displays with panel offset in RAM)
    _colOffset = dispOpts.spiColOffset;
    _rowOffset = dispOpts.spiRowOffset;

    // Read display metrics
    GPGFX_DisplayMetrics* metrics = getMetrics();
    if (metrics != nullptr) {
        frameWidth = metrics->width;
        frameHeight = metrics->height;
    }

    // Determine SPI hardware instance directly from spiBlock
    // (bypass PeripheralSPI which may have wrong/default hardware pointer)
    uint8_t block = (uint8_t)dispOpts.spiBlock;
    _spiHW = (block == 0) ? spi0 : spi1;

    // Directly initialize SPI hardware with Pico SDK (bypass PeripheralSPI)
    spi_init(_spiHW, DISPLAY_SPI_SPEED);
    spi_set_format(_spiHW, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Configure SCK/TX pins as SPI function
    if (sck != (uint8_t)-1) gpio_set_function(sck, GPIO_FUNC_SPI);
    if (tx  != (uint8_t)-1) gpio_set_function(tx,  GPIO_FUNC_SPI);

    // Initialize control GPIOs
    if (_pinDC != (uint8_t)-1) {
        gpio_init(_pinDC);
        gpio_set_dir(_pinDC, GPIO_OUT);
        gpio_put(_pinDC, 0);
    }

    if (_pinCS != (uint8_t)-1) {
        gpio_init(_pinCS);
        gpio_set_dir(_pinCS, GPIO_OUT);
        gpio_put(_pinCS, 1);
    }

    if (_pinRST != (uint8_t)-1) {
        gpio_init(_pinRST);
        gpio_set_dir(_pinRST, GPIO_OUT);
        gpio_put(_pinRST, 1);
    }

    if (_pinBL != (int8_t)-1) {
        gpio_init((uint8_t)_pinBL);
        gpio_set_dir((uint8_t)_pinBL, GPIO_OUT);
        gpio_put((uint8_t)_pinBL, 1);
    }

    hardwareReset();

    // Re-assert SPI pins after hardware reset (belt and suspenders)
    if (sck != (uint8_t)-1) gpio_set_function(sck, GPIO_FUNC_SPI);
    if (tx  != (uint8_t)-1) gpio_set_function(tx,  GPIO_FUNC_SPI);

    // Allocate framebuffer
    frameBuffer = new uint16_t[frameWidth * frameHeight];
    if (frameBuffer == nullptr) return;

    // ST7789 init sequence (minimal, matching working test program)
    writeCommand(CommandOps::SWRESET);
    sleep_ms(200);

    writeCommand(CommandOps::SLPOUT);
    sleep_ms(200);

    writeCommand(CommandOps::MADCTL);
    // Read vertical flip setting
    bool flipV = Storage::getInstance().getDisplayOptions().spiFlipVertical;

    // ST7735 landscape: 160x80 framebuffer, MV=1, BGR, +0° rotation
    // ST7735 needs BGR (0x08) unlike ST7789. Panel offsets vary by module.
    uint8_t madctl = 0x28; // MV=0x20 + BGR=0x08
    switch (_options.orientation) {
        case 0: break;               // 0x28 — MV + BGR
        case 1: madctl |= 0x40; break;  // MV + BGR + MX
        case 2: madctl |= 0xC0; break;  // MV + BGR + MX + MY
        case 3: madctl |= 0x80; break;  // MV + BGR + MY
    }
    if (flipV) madctl ^= 0x80;
    writeData(madctl);

    // COLMOD for ST7735: 0x05 = 16-bit/pixel 65K colors
    writeCommand(CommandOps::COLMOD);
    writeData(0x05);

    // Use webconfig offsets, default 0
    _colOffset = dispOpts.spiColOffset;
    _rowOffset = dispOpts.spiRowOffset;

    writeCommand(CommandOps::INVOFF);
    writeCommand(CommandOps::NORON);
    writeCommand(CommandOps::DISPON);
    sleep_ms(200);

    _isPowered = true;

    // Boot animation: pink hearts converge then spiral burst
    const int W = frameWidth, H = frameHeight;
    const uint16_t pink  = 0xF811;  // hot pink
    const uint16_t pink2 = 0xF8B2;  // light pink
    const uint16_t red   = 0xF800;  // red
    const uint16_t white = 0xFFFF;
    // Heart shape (10x8 grid, proper heart)
    const char heart[] = {
        0,1,1,0,0,1,1,0,0,0,
        1,1,1,1,1,1,1,1,0,0,
        1,1,1,1,1,1,1,1,1,0,
        0,1,1,1,1,1,1,1,1,0,
        0,0,1,1,1,1,1,1,0,0,
        0,0,0,1,1,1,1,0,0,0,
        0,0,0,0,1,1,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
    };
    const int HW = 10, HH = 8;
    for (int t = 0; t < 40; t++) {
        memset(frameBuffer, 0, W * H * 2);
        float progress = (float)t / 39.0f;
        for (int h = 0; h < 8; h++) {
            float angle = (float)(h * 45 + t * 5) * 3.14159f / 180.0f;
            float dist = (1.0f - progress) * (float)(W + H) / 2.4f;
            int sx = (int)(W/2.0f + cosf(angle) * dist);
            int sy = (int)(H/2.0f + sinf(angle) * dist * 0.65f);
            int size = 3 + (int)(progress * 3.5f);
            uint16_t hc = (h % 3 == 0) ? pink : (h % 3 == 1 ? pink2 : red);
            for (int dy = 0; dy < HH; dy++) {
                for (int dx = 0; dx < HW; dx++) {
                    if (heart[dy*HW+dx]) {
                        for (int sy2 = 0; sy2 < size; sy2++)
                            for (int sx2 = 0; sx2 < size; sx2++) {
                                int px = sx + dx * size + sx2 - (HW/2*size);
                                int py = sy + dy * size + sy2 - (HH/2*size);
                                if (px >= 0 && px < W && py >= 0 && py < H)
                                    frameBuffer[py * W + px] = hc;
                            }
                    }
                }
            }
        }
        // Center glow
        if (progress > 0.3f) {
            int glowR = (int)((progress - 0.3f) * 45.0f);
            for (int dy = -glowR; dy <= glowR; dy++)
                for (int dx = -glowR; dx <= glowR; dx++) {
                    int px = W/2 + dx, py = H/2 + dy;
                    if (px >= 0 && px < W && py >= 0 && py < H && (dx*dx+dy*dy) <= glowR*glowR)
                        frameBuffer[py*W+px] = white;
                }
        }
        drawBuffer(nullptr);
        sleep_ms(50);
    }
    // White flash
    for (size_t i = 0; i < (size_t)W*H; i++) frameBuffer[i] = white;
    drawBuffer(nullptr); sleep_ms(50);
    // Phase 2: spiral burst from center (seamless from white flash)
    for (int t = 0; t < 35; t++) {
        memset(frameBuffer, 0, W * H * 2);
        int cx = W/2, cy = H/2;
        for (int i = 0; i < 100; i++) {
            float a = (float)(i * 137 + t * 15) * 3.14159f / 180.0f;
            int r = t * 5 + i / 3;
            int px = cx + (int)(r * cosf(a));
            int py = cy + (int)(r * sinf(a));
            if (px >= 0 && px < W && py >= 0 && py < H) {
                uint8_t hue = (t * 8 + i * 3) % 256;
                uint8_t r8 = hue < 85 ? 255 - hue*3 : (hue < 170 ? 0 : (hue-170)*3);
                uint8_t g8 = hue < 85 ? hue*3 : (hue < 170 ? 255-(hue-85)*3 : 0);
                uint8_t b8 = hue < 85 ? 0 : (hue < 170 ? (hue-85)*3 : 255-(hue-170)*3);
                frameBuffer[py * W + px] = ((r8>>3)<<11)|((g8>>2)<<5)|(b8>>3);
            }
        }
        drawBuffer(nullptr);
        sleep_ms(18);
    }
    // Phase 3: quick fade
    for (int f = 0; f < 5; f++) {
        for (size_t i = 0; i < (size_t)W * H; i++) {
            uint16_t c = frameBuffer[i];
            int r = (c>>11)&0x1F, g = (c>>5)&0x3F, b = c&0x1F;
            r = r * (4-f) / 4; g = g * (4-f) / 4; b = b * (4-f) / 4;
            frameBuffer[i] = (r<<11)|(g<<5)|b;
        }
        drawBuffer(nullptr);
        sleep_ms(25);
    }
    clear();
    drawBuffer(nullptr);

}

void GPGFX_ST7735::hardwareReset() {
    if (_pinRST == (uint8_t)-1) return;
    gpio_put(_pinRST, 0);
    sleep_ms(10);
    gpio_put(_pinRST, 1);
    sleep_ms(120);
}

void GPGFX_ST7735::setPower(bool isPowered) {
    _isPowered = isPowered;
    if (_spi == nullptr) return;
    writeCommand(isPowered ? CommandOps::DISPON : CommandOps::DISPOFF);
    if (_pinBL != (int8_t)-1) {
        gpio_put((uint8_t)_pinBL, isPowered ? 1 : 0);
    }
}

void GPGFX_ST7735::clear() {
    if (frameBuffer == nullptr) return;
    memset(frameBuffer, 0, frameWidth * frameHeight * sizeof(uint16_t));
}

uint32_t GPGFX_ST7735::getPixel(uint8_t x, uint8_t y) {
    if (frameBuffer == nullptr) return 0;
    if (x >= frameWidth || y >= frameHeight) return 0;
    return frameBuffer[y * frameWidth + x];
}

void GPGFX_ST7735::drawPixel(uint8_t x, uint8_t y, uint32_t color) {
    if (frameBuffer == nullptr) return;
    if (x >= frameWidth || y >= frameHeight) return;

    // Map monochrome-style colors to configured RGB565 colors
    const DisplayOptions& opts = Storage::getInstance().getDisplayOptions();
    uint16_t textColor = (uint16_t)(opts.spiTextColor & 0xFFFF);
    uint16_t bgColor = (uint16_t)(opts.spiBgColor & 0xFFFF);

    if (color == 1) {
        color = (_overrideColor != 0) ? _overrideColor : ((textColor != 0) ? textColor : (uint16_t)0xFFFF);
    } else if (color == 0) {
        color = (bgColor != 0 || opts.spiBgColor == 0) ? bgColor : 0x0000;
    }

    frameBuffer[y * frameWidth + x] = (uint16_t)(color & 0xFFFF);
}

void GPGFX_ST7735::drawText(uint8_t x, uint8_t y, std::string text, uint8_t invert) {
    if (frameBuffer == nullptr) return;

    uint8_t charOffset = 0;
    uint8_t maxTextSize = (frameWidth / _options.font.width);

    for (uint8_t charIndex = 0; charIndex < MIN(text.size(), maxTextSize); charIndex++) {
        uint8_t currChar = text[charIndex];
        uint8_t glyphIndex = currChar - GPGFX_FONT_CHAR_OFFSET;
        const uint8_t* currGlyph = &_options.font.fontData[glyphIndex * ((_options.font.width - 1) * (_options.font.height / 8))];

        for (uint8_t spriteY = 0; spriteY < _options.font.height; spriteY++) {
            for (uint8_t spriteX = 0; spriteX < _options.font.width - 1; spriteX++) {
                uint8_t spriteBit = spriteY % 8;
                uint8_t spriteByte = currGlyph[spriteX];
                uint16_t color = ((spriteByte >> spriteBit) & 0x01);

                if (color) {
                    drawPixel((x * _options.font.width) + spriteX + charOffset, (y * _options.font.height) + spriteY, invert ? 0 : 1);
                } else if (invert) {
                    drawPixel((x * _options.font.width) + spriteX + charOffset, (y * _options.font.height) + spriteY, 1);
                }
            }
        }

        charOffset += _options.font.width;
    }
}

void GPGFX_ST7735::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color, uint8_t filled) {
    int dx = abs((int)x2 - (int)x1);
    int dy = abs((int)y2 - (int)y1);
    int stepX = (x1 < x2) ? 1 : -1;
    int stepY = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int errDouble = 2 * err;
        if (errDouble > -dy) { err -= dy; x1 += stepX; }
        if (errDouble < dx)  { err += dx; y1 += stepY; }
    }
}

void GPGFX_ST7735::drawArc(uint16_t x, uint16_t y, uint32_t radiusX, uint32_t radiusY, uint32_t color, uint8_t filled, double startAngle, double endAngle, uint8_t closed) {
    startAngle = startAngle * M_PI / 180.0;
    endAngle = endAngle * M_PI / 180.0;
    double angleStep = 0.01;

    for (double angle = startAngle; angle < endAngle; angle += angleStep) {
        drawPixel(x + static_cast<int>(radiusX * cos(angle)), y + static_cast<int>(radiusY * sin(angle)), color);
    }
    drawPixel(x + static_cast<int>(radiusX * cos(endAngle)), y + static_cast<int>(radiusY * sin(endAngle)), color);

    if (closed) {
        drawLine(x, y, x + static_cast<int>(radiusX * cos(startAngle)), y + static_cast<int>(radiusY * sin(startAngle)), color, filled);
        drawLine(x, y, x + static_cast<int>(radiusX * cos(endAngle)), y + static_cast<int>(radiusY * sin(endAngle)), color, filled);
    }
    if (filled) {
        for (double angle = startAngle; angle <= endAngle; angle += angleStep) {
            drawLine(x, y, x + static_cast<int>(radiusX * cos(angle)), y + static_cast<int>(radiusY * sin(angle)), color, filled);
        }
    }
}

void GPGFX_ST7735::drawEllipse(uint16_t x, uint16_t y, uint32_t radiusX, uint32_t radiusY, uint32_t color, uint8_t filled) {
    long x1 = -(long)radiusX, y1 = 0;
    long e2 = (long)radiusY;
    long dx = (1 + 2 * x1) * e2 * e2;
    long dy = x1 * x1;
    long err = dx + dy;
    long rx = (long)radiusX;
    long ry = (long)radiusY;

    while (x1 <= 0) {
        drawPixel(x - x1, y + y1, color);
        drawPixel(x + x1, y + y1, color);
        drawPixel(x + x1, y - y1, color);
        drawPixel(x - x1, y - y1, color);

        if (filled) {
            for (int i = 0; i < ((x - x1) - (x + x1)) / 2; i++) {
                drawPixel(x - i, y + y1, color);
                drawPixel(x + i, y + y1, color);
                drawPixel(x + i, y - y1, color);
                drawPixel(x - i, y - y1, color);
            }
        }

        e2 = 2 * err;
        if (e2 >= dx) { x1++; err += dx += 2 * ry * ry; }
        if (e2 <= dy) { y1++; err += dy += 2 * rx * rx; }
    }

    while (y1++ < ry) {
        drawPixel(x, y + y1, color);
        drawPixel(x, y - y1, color);
    }
}

void GPGFX_ST7735::drawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color, uint8_t filled, double rotationAngle) {
    if (rotationAngle == 0.0 && filled) {
        for (uint16_t yi = y; yi <= height; yi++) {
            for (uint16_t xi = x; xi <= width; xi++) {
                drawPixel(xi, yi, color);
            }
        }
        return;
    }

    double centerX = (x + width) / 2.0;
    double centerY = (y + height) / 2.0;
    double halfWidth = (width - x) / 2.0;
    double halfHeight = (height - y) / 2.0;
    double angleRad = rotationAngle * M_PI / 180.0;
    double cosA = cos(angleRad);
    double sinA = sin(angleRad);

    double corners[4][2] = {
        {cosA * (-halfWidth) - sinA * (-halfHeight), sinA * (-halfWidth) + cosA * (-halfHeight)},
        {cosA * (halfWidth) - sinA * (-halfHeight),  sinA * (halfWidth) + cosA * (-halfHeight)},
        {cosA * (halfWidth) - sinA * (halfHeight),   sinA * (halfWidth) + cosA * (halfHeight)},
        {cosA * (-halfWidth) - sinA * (halfHeight),  sinA * (-halfWidth) + cosA * (halfHeight)}
    };

    uint16_t rx[4], ry[4];
    for (int i = 0; i < 4; i++) {
        rx[i] = (uint16_t)round(centerX + corners[i][0]);
        ry[i] = (uint16_t)round(centerY + corners[i][1]);
    }

    drawLine(rx[0], ry[0], rx[1], ry[1], color, filled);
    drawLine(rx[1], ry[1], rx[2], ry[2], color, filled);
    drawLine(rx[2], ry[2], rx[3], ry[3], color, filled);
    drawLine(rx[3], ry[3], rx[0], ry[0], color, filled);

    if (filled) {
        uint16_t numLines = (uint16_t)round(sqrt(halfWidth * halfWidth + halfHeight * halfHeight) * 2);
        for (uint16_t i = 0; i <= numLines; i++) {
            double t = (double)i / numLines;
            double xStart = (1 - t) * corners[0][0] + t * corners[3][0];
            double yStart = (1 - t) * corners[0][1] + t * corners[3][1];
            double xEnd = (1 - t) * corners[1][0] + t * corners[2][0];
            double yEnd = (1 - t) * corners[1][1] + t * corners[2][1];
            drawLine((uint16_t)round(centerX + xStart), (uint16_t)round(centerY + yStart),
                     (uint16_t)round(centerX + xEnd), (uint16_t)round(centerY + yEnd), color, 1);
        }
    }
}

void GPGFX_ST7735::drawPolygon(uint16_t x, uint16_t y, uint16_t radius, uint16_t sides, uint32_t color, uint8_t filled, double rotation) {
    double angleIncrement = 2 * M_PI / sides;
    uint16_t xVertices[16], yVertices[16];
    sides = MIN(sides, 16);

    for (int i = 0; i < sides; i++) {
        double angle = i * angleIncrement + rotation;
        xVertices[i] = x + round(radius * cos(angle));
        yVertices[i] = y + round(radius * sin(angle));
    }

    for (int i = 0; i < sides - 1; i++) {
        drawLine(xVertices[i], yVertices[i], xVertices[i + 1], yVertices[i + 1], color, false);
    }
    drawLine(xVertices[sides - 1], yVertices[sides - 1], xVertices[0], yVertices[0], color, false);

    if (filled) {
        uint16_t minY = yVertices[0], maxY = yVertices[0];
        for (int i = 1; i < sides; i++) {
            if (yVertices[i] < minY) minY = yVertices[i];
            if (yVertices[i] > maxY) maxY = yVertices[i];
        }

        for (uint16_t scanY = minY + 1; scanY < maxY; scanY++) {
            int intersections = 0;
            double intersectPoints[16];
            for (int i = 0; i < sides; i++) {
                int next = (i + 1) % sides;
                if ((yVertices[i] < scanY && yVertices[next] >= scanY) || (yVertices[next] < scanY && yVertices[i] >= scanY)) {
                    intersectPoints[intersections++] = xVertices[i] + (double)(scanY - yVertices[i]) * (xVertices[next] - xVertices[i]) / (yVertices[next] - yVertices[i]);
                }
            }
            for (int i = 0; i < intersections - 1; i++) {
                for (int j = 0; j < intersections - i - 1; j++) {
                    if (intersectPoints[j] > intersectPoints[j + 1]) {
                        double temp = intersectPoints[j];
                        intersectPoints[j] = intersectPoints[j + 1];
                        intersectPoints[j + 1] = temp;
                    }
                }
            }
            for (int i = 0; i < intersections; i += 2) {
                drawLine((uint16_t)intersectPoints[i], scanY, (uint16_t)intersectPoints[i + 1], scanY, color, 1);
            }
        }
    }
}

void GPGFX_ST7735::drawPill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color, uint8_t filled, double rotationAngle) {
    bool horizontal = (width - x) >= (height - y);

    double centerX = (x + width) / 2.0;
    double centerY = (y + height) / 2.0;
    double halfWidth = (width - x) / 2.0;
    double halfHeight = (height - y) / 2.0;
    double radius = horizontal ? halfHeight : halfWidth;
    double rectHalfLong = (horizontal ? halfWidth : halfHeight) - radius;
    double rectHalfShort = horizontal ? halfHeight : halfWidth;

    double angleRad = rotationAngle * M_PI / 180.0;
    double cosA = cos(angleRad);
    double sinA = sin(angleRad);

    double longX = horizontal ? 1.0 : 0.0;
    double longY = horizontal ? 0.0 : 1.0;
    double shortX = -longY;
    double shortY = longX;

    double corners[4][2] = {
        {-rectHalfLong * longX - rectHalfShort * shortX, -rectHalfLong * longY - rectHalfShort * shortY},
        { rectHalfLong * longX - rectHalfShort * shortX,  rectHalfLong * longY - rectHalfShort * shortY},
        { rectHalfLong * longX + rectHalfShort * shortX,  rectHalfLong * longY + rectHalfShort * shortY},
        {-rectHalfLong * longX + rectHalfShort * shortX, -rectHalfLong * longY + rectHalfShort * shortY}
    };

    uint16_t rx[4], ry[4];
    for (int i = 0; i < 4; i++) {
        rx[i] = (uint16_t)round(centerX + cosA * corners[i][0] - sinA * corners[i][1]);
        ry[i] = (uint16_t)round(centerY + sinA * corners[i][0] + cosA * corners[i][1]);
    }

    drawLine(rx[0], ry[0], rx[1], ry[1], color, filled);
    drawLine(rx[2], ry[2], rx[3], ry[3], color, filled);

    if (filled) {
        double longVecX = cosA * longX - sinA * longY;
        double longVecY = sinA * longX + cosA * longY;
        double shortVecX = cosA * shortX - sinA * shortY;
        double shortVecY = sinA * shortX + cosA * shortY;

        int steps = (int)ceil(2.0 * rectHalfLong * 8.0);
        if (steps < 1) steps = 1;

        for (int k = -steps; k <= steps; k++) {
            double t = (double)k / steps;
            double baseX = centerX + t * rectHalfLong * longVecX;
            double baseY = centerY + t * rectHalfLong * longVecY;
            double sx = rectHalfShort * shortVecX + 0.5 * (shortVecX >= 0 ? 1 : -1);
            double sy = rectHalfShort * shortVecY + 0.5 * (shortVecY >= 0 ? 1 : -1);
            drawLine((uint16_t)round(baseX - sx), (uint16_t)round(baseY - sy), (uint16_t)round(baseX + sx), (uint16_t)round(baseY + sy), color, 1);
        }
    }

    int steps = 32;
    for (int side = -1; side <= 1; side += 2) {
        double cx = side * rectHalfLong * longX;
        double cy = side * rectHalfLong * longY;
        double prevX = 0, prevY = 0;

        for (int i = 0; i <= steps; i++) {
            double t = (double)i / steps;
            double theta = -M_PI / 2 + t * M_PI;
            if (side == -1) theta = M_PI / 2 + t * M_PI;

            double thetaOffset = horizontal ? -M_PI / 2 : M_PI / 2;
            double arcLocalX = cx + radius * cos(theta + thetaOffset) * shortX + radius * sin(theta + thetaOffset) * longX;
            double arcLocalY = cy + radius * cos(theta + thetaOffset) * shortY + radius * sin(theta + thetaOffset) * longY;

            double rotatedX = centerX + cosA * arcLocalX - sinA * arcLocalY;
            double rotatedY = centerY + sinA * arcLocalX + cosA * arcLocalY;

            if (i > 0) {
                drawLine((uint16_t)round(rotatedX), (uint16_t)round(rotatedY), (uint16_t)round(prevX), (uint16_t)round(prevY), color, filled);
                if (filled) {
                    double capCenterX = centerX + cosA * cx - sinA * cy;
                    double capCenterY = centerY + sinA * cx + cosA * cy;
                    drawLine((uint16_t)round(rotatedX), (uint16_t)round(rotatedY), (uint16_t)round(capCenterX), (uint16_t)round(capCenterY), color, 1);
                }
            }
            prevX = rotatedX;
            prevY = rotatedY;
        }
    }
}

void GPGFX_ST7735::drawSprite(uint8_t* spriteData, uint16_t width, uint16_t height, uint16_t pitch, uint16_t x, uint16_t y, uint8_t priority, double scale) {
    if (frameBuffer == nullptr || spriteData == nullptr) return;

    for (uint16_t scaledY = 0; scaledY < (uint16_t)(height * scale); ++scaledY) {
        for (uint16_t scaledX = 0; scaledX < (uint16_t)(width * scale); ++scaledX) {
            uint8_t spriteX = (uint8_t)(scaledX / scale);
            uint8_t spriteY = (uint8_t)(scaledY / scale);
            uint8_t spriteBit = spriteX % 8;
            uint8_t spriteByte = spriteData[(spriteY * ((width + 7) / 8)) + (spriteX / 8)];
            uint8_t colorBit = ((spriteByte >> (7 - spriteBit)) & 0x01);
            drawPixel(x + scaledX, y + scaledY, colorBit ? 1 : 0);
        }
    }
}

void GPGFX_ST7735::drawBuffer(uint8_t* pBuffer) {
    if (_spiHW == nullptr) return;
    if (frameBuffer == nullptr && pBuffer == nullptr) return;

    uint16_t* src = (pBuffer != nullptr) ? (uint16_t*)pBuffer : frameBuffer;
    if (src == nullptr) return;

    setAddrWindow(0, 0, frameWidth - 1, frameHeight - 1);
    writeCommand(CommandOps::RAMWR);

    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 0);
    gpio_put(_pinDC, 1);

    // Write pixels one at a time, matching working test program exactly
    size_t totalPixels = (size_t)frameWidth * frameHeight;
    for (size_t i = 0; i < totalPixels; i++) {
        uint8_t buf[2] = { (uint8_t)(src[i] >> 8), (uint8_t)(src[i] & 0xFF) };
        spi_write_blocking(_spiHW, buf, 2);
    }

    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 1);
}

void GPGFX_ST7735::writeCommand(uint8_t cmd) {
    if (_spiHW == nullptr) return;
    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 0);
    gpio_put(_pinDC, 0);
    spi_write_blocking(_spiHW, &cmd, 1);
    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 1);
}

void GPGFX_ST7735::writeData(uint8_t data) {
    if (_spiHW == nullptr) return;
    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 0);
    gpio_put(_pinDC, 1);
    spi_write_blocking(_spiHW, &data, 1);
    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 1);
}

void GPGFX_ST7735::writeData16(uint16_t data) {
    if (_spiHW == nullptr) return;
    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 0);
    gpio_put(_pinDC, 1);
    uint16_t beData = __builtin_bswap16(data);
    spi_write_blocking(_spiHW, (const uint8_t*)&beData, 2);
    if (_pinCS != (uint8_t)-1) gpio_put(_pinCS, 1);
}

void GPGFX_ST7735::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    writeCommand(CommandOps::CASET);
    writeData16(x0 + _colOffset);
    writeData16(x1 + _colOffset);

    writeCommand(CommandOps::RASET);
    writeData16(y0 + _rowOffset);
    writeData16(y1 + _rowOffset);
}
