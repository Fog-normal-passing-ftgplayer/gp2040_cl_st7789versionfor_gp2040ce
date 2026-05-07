#ifndef _GPGFX_ST7789_H_
#define _GPGFX_ST7789_H_

#include "GPGFX_types.h"
#include "displaybase.h"
#include "math.h"

class GPGFX_ST7789 : public GPGFX_DisplayBase {
    public:
        GPGFX_ST7789() {}
        ~GPGFX_ST7789();

        void init(GPGFX_DisplayTypeOptions options);
        void setPower(bool isPowered);
        void clear();

        uint32_t getPixel(uint8_t x, uint8_t y);
        void drawPixel(uint8_t x, uint8_t y, uint32_t color);
        void drawText(uint8_t x, uint8_t y, std::string text, uint8_t invert = 0);
        void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color, uint8_t filled);
        void drawArc(uint16_t x, uint16_t y, uint32_t radiusX, uint32_t radiusY, uint32_t color, uint8_t filled, double startAngle, double endAngle, uint8_t closed);
        void drawEllipse(uint16_t x, uint16_t y, uint32_t radiusX, uint32_t radiusY, uint32_t color, uint8_t filled);
        void drawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color, uint8_t filled, double rotationAngle = 0);
        void drawPolygon(uint16_t x, uint16_t y, uint16_t radius, uint16_t sides, uint32_t color, uint8_t filled, double rotation = 0);
        void drawPill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color, uint8_t filled, double rotationAngle = 0);
        void drawSprite(uint8_t* spriteData, uint16_t width, uint16_t height, uint16_t pitch, uint16_t x, uint16_t y, uint8_t priority, double scale = 1.0);
        void drawBuffer(uint8_t *pBuffer);

        void runTestPattern();
        void setOverrideColor(uint16_t c) { _overrideColor = c; }
        void blitFrame(const uint16_t* frame) {
            if (frameBuffer && frame) memcpy(frameBuffer, frame, frameWidth * frameHeight * 2);
        }

        bool isSPI() { return true; }
        bool isI2C() { return false; }

        std::vector<uint8_t> getDeviceAddresses() const override {
            return {};
        }

    private:
        typedef enum {
            NOP       = 0x00,
            SWRESET   = 0x01,
            RDDID     = 0x04,
            RDDST     = 0x09,
            SLPIN     = 0x10,
            SLPOUT    = 0x11,
            PTLON     = 0x12,
            NORON     = 0x13,
            INVOFF    = 0x20,
            INVON     = 0x21,
            DISPOFF   = 0x28,
            DISPON    = 0x29,
            CASET     = 0x2A,
            RASET     = 0x2B,
            RAMWR     = 0x2C,
            RAMRD     = 0x2E,
            PTLAR     = 0x30,
            COLMOD    = 0x3A,
            MADCTL    = 0x36,
            FRMCTR1   = 0xB1,
            FRMCTR2   = 0xB2,
            FRMCTR3   = 0xB3,
            INVCTR    = 0xB4,
            PWCTRL1   = 0xC0,
            PWCTRL2   = 0xC1,
            PWCTRL3   = 0xC2,
            PWCTRL4   = 0xC3,
            PWCTRL5   = 0xC4,
            VMCTR1    = 0xC5,
            VMCTR2    = 0xC7,
            GMCTRP1   = 0xE0,
            GMCTRN1   = 0xE1,
        } CommandOps;

        GPGFX_DisplayTypeOptions _options;

        PeripheralSPI* _spi = nullptr;
        spi_inst_t* _spiHW = nullptr;
        uint8_t _pinDC;
        uint8_t _pinRST;
        int8_t _pinBL;
        uint8_t _pinCS;

        bool _isPowered = false;

        uint16_t* frameBuffer = nullptr;
        uint16_t frameWidth = 135;
        uint16_t frameHeight = 240;
        uint16_t _colOffset = 0;
        uint16_t _rowOffset = 0;
        uint16_t _overrideColor = 0;

        void writeCommand(uint8_t cmd);
        void writeData(uint8_t data);
        void writeData16(uint16_t data);
        void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
        void hardwareReset();
};

// Global test function callable from API handlers
void st7789_runTestPattern();

#endif
