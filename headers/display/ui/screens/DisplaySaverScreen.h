#ifndef _DISPLAYSAVERSCREEN_H_
#define _DISPLAYSAVERSCREEN_H_

#include <vector>
#include "GPGFX_UI_widgets.h"

#define MAX_SAVER_W 240
#define MAX_SAVER_H 240

class DisplaySaverScreen : public GPScreen {
    public:
        DisplaySaverScreen() {}
        DisplaySaverScreen(GPGFX* renderer) { setRenderer(renderer); }
        virtual ~DisplaySaverScreen() {}
        virtual int8_t update();
        virtual void init();
        virtual void shutdown();
    protected:
        virtual void drawScreen();
        uint16_t prevButtonState = 0;
        DisplaySaverMode displaySaverMode{};
        uint16_t screenW = 128, screenH = 64;

        // snow screen
        uint8_t snowflakeSpeeds[MAX_SAVER_W][MAX_SAVER_H] = {};
        int8_t snowflakeDrift[MAX_SAVER_W][MAX_SAVER_H] = {};
        void initSnowScene();
        void drawSnowScene();

        // bounce
        uint16_t bounceSpriteX = 0;
        uint16_t bounceSpriteY = 0;
        uint16_t bounceSpriteWidth = 128;
        uint16_t bounceSpriteHeight = 35;
        double bounceSpriteVelocityX = 1;
        double bounceSpriteVelocityY = 1;
        double bounceScale = 0.5;
        void drawBounceScene();

        // pipes
        void drawPipeScene();

        // toaster
        struct ToastParams {
            double scale;
            int16_t x;
            int16_t y;
            int16_t dx;
            int16_t dy;
        };

        std::vector<ToastParams> toasters;
        uint16_t numberOfToasters = 10;
        uint16_t toasterSpriteWidth = 128;
        uint16_t toasterSpriteHeight = 35;
        void initToasters();
        void drawToasterScene();
        void drawNyancatScene();
        uint8_t nyancatFrame = 0;
        uint32_t nyancatTick = 0;

        void delay_us(uint32_t us);

        const uint32_t SCREEN_DELAY_PIPES = 30;
};

#endif