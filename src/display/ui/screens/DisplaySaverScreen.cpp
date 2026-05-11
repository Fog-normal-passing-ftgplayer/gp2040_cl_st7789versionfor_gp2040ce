#include "DisplaySaverScreen.h"
#include "BitmapScreens.h"
#include "NyancatFrames.h"

#include "drivermanager.h"
#include "pico/stdlib.h"
#include "version.h"
#include <cstring>

void DisplaySaverScreen::init() {
    const DisplayOptions& options = Storage::getInstance().getDisplayOptions();
    displaySaverMode = options.displaySaverMode;
    screenW = getRenderer()->getDriver()->getMetrics()->width;
    screenH = getRenderer()->getDriver()->getMetrics()->height;
    if (screenW > MAX_SAVER_W) screenW = MAX_SAVER_W;
    if (screenH > MAX_SAVER_H) screenH = MAX_SAVER_H;

    getRenderer()->clearScreen();

    switch (displaySaverMode) {
        case DisplaySaverMode::DISPLAY_SAVER_SNOW:
            initSnowScene();
            break;
        case DisplaySaverMode::DISPLAY_SAVER_BOUNCE:
            break;
        case DisplaySaverMode::DISPLAY_SAVER_PIPES:
            break;
        case DisplaySaverMode::DISPLAY_SAVER_TOAST:
            initToasters();
            break;
        case 5:
            nyancatFrame = 0; nyancatTick = 0;
            break;
        default:
            break;
    }
}

void DisplaySaverScreen::shutdown() {
    clearElements();
}

void DisplaySaverScreen::drawScreen() {
    switch (displaySaverMode) {
        case DisplaySaverMode::DISPLAY_SAVER_SNOW:
            drawSnowScene();
            break;
        case DisplaySaverMode::DISPLAY_SAVER_BOUNCE:
            drawBounceScene();
            break;
        case DisplaySaverMode::DISPLAY_SAVER_PIPES:
            drawPipeScene();
            break;
        case DisplaySaverMode::DISPLAY_SAVER_TOAST:
            drawToasterScene();
            break;
        case 5:
            drawNyancatScene();
            break;
        default:
            break;
    }
}

int8_t DisplaySaverScreen::update() {
    if (!DriverManager::getInstance().isConfigMode()) {
        Gamepad * gamepad = Storage::getInstance().GetGamepad();
        
        if (gamepad->state.buttons || gamepad->state.dpad) {
            return DisplayMode::BUTTONS;
        }
    }

    return -1; // -1 means no change in screen state
}

void DisplaySaverScreen::initSnowScene() {
    for (uint8_t x = 0; x < screenW; ++x) {
        for (uint8_t y = 0; y < screenH; ++y) {
            snowflakeSpeeds[x][y] = 0;
            snowflakeDrift[x][y] = 0;
            getRenderer()->drawPixel(x, y, 0);
        }
    }
}

void DisplaySaverScreen::drawSnowScene() {
    for (int8_t y = screenH - 1; y >= 0; --y) {
        for (uint8_t x = 0; x < screenW; ++x) {
            if (snowflakeSpeeds[x][y] > 0) {
                uint8_t speed = snowflakeSpeeds[x][y];
                uint8_t newY = y + speed;
                int8_t drift = snowflakeDrift[x][y];
                int8_t newX = x + drift;

                if (newX < 0) newX = 0;
                if (newX >= screenW) newX = screenW - 1;

                if (newY >= screenH) {
                    getRenderer()->drawPixel(x, y, 0);
                    snowflakeSpeeds[x][y] = 0;
                    snowflakeDrift[x][y] = 0;
                } else {
                    getRenderer()->drawPixel(x, y, 0);
                    getRenderer()->drawPixel(newX, newY, 1);
                    snowflakeSpeeds[newX][newY] = speed;
                    snowflakeDrift[newX][newY] = drift;
                    snowflakeSpeeds[x][y] = 0;
                    snowflakeDrift[x][y] = 0;
                }
            }
        }
    }

    for (uint8_t x = 0; x < screenW; ++x) {
        if (rand() % 10 == 0) {
            getRenderer()->drawPixel(x, 0, 1);
            snowflakeSpeeds[x][0] = (rand() % 3) + 1;
            snowflakeDrift[x][0] = (rand() % 3) - 1;
        }
    }
}

void DisplaySaverScreen::drawBounceScene() {
    uint16_t scaledWidth = static_cast<uint16_t>(bounceSpriteWidth * bounceScale);
    uint16_t scaledHeight = static_cast<uint16_t>(bounceSpriteHeight * bounceScale);

    bounceSpriteX += bounceSpriteVelocityX;
    bounceSpriteY += bounceSpriteVelocityY;

    if (bounceSpriteX <= 0 || bounceSpriteX + scaledWidth >= screenW) bounceSpriteVelocityX = -bounceSpriteVelocityX;

    if (bounceSpriteY <= 0 || bounceSpriteY + scaledHeight >= screenH) bounceSpriteVelocityY = -bounceSpriteVelocityY;

    getRenderer()->drawSprite((uint8_t *)bitmapGP2040Logo, bounceSpriteWidth, bounceSpriteHeight, 0, bounceSpriteX, bounceSpriteY, 0, bounceScale);
}

void DisplaySaverScreen::drawPipeScene() {
    const uint8_t PIPE_WIDTH = 4;
    const uint8_t PIPE_COLOR = 1;

    uint8_t currentX = 0;
    uint8_t currentY = 0;

    while (currentY < screenH) {
        bool connectRight = rand() % 2;
        bool connectDown = rand() % 2;

        if (connectRight && currentX + PIPE_WIDTH < screenW) {
            for (uint8_t i = 0; i < PIPE_WIDTH; ++i) {
                getRenderer()->drawPixel(currentX + i, currentY, PIPE_COLOR);
            }
        }

        if (connectDown && currentY + PIPE_WIDTH < screenH) {
            for (uint8_t i = 0; i < PIPE_WIDTH; ++i) {
                getRenderer()->drawPixel(currentX, currentY + i, PIPE_COLOR);
            }
        }

        getRenderer()->drawPixel(currentX, currentY, PIPE_COLOR);

        currentX += PIPE_WIDTH;
        if (currentX >= screenW) {
            currentX = 0;
            currentY += PIPE_WIDTH;
        }

        delay_us(SCREEN_DELAY_PIPES);
    }
}

void DisplaySaverScreen::initToasters() {
    for (uint16_t i = 0; i < numberOfToasters; ++i) {
        double scale = 1.0 / ((i/2)+2);
        int16_t dx = (-1 - rand() % 2);
        int16_t dy = (1 + rand() % 2);

        toasters.push_back({
            scale,
            static_cast<int16_t>(screenW - toasterSpriteWidth * scale),
            static_cast<int16_t>(rand() % (screenH - static_cast<int16_t>(toasterSpriteHeight * scale))),
            static_cast<int16_t>(dx),
            static_cast<int16_t>(dy)
        });
    }
}

void DisplaySaverScreen::drawToasterScene() {
    for (uint16_t i = 0; i < toasters.size(); ++i) {
        ToastParams& sprite = toasters[i];

        getRenderer()->drawSprite((uint8_t *)bitmapGP2040Logo, toasterSpriteWidth, toasterSpriteHeight, 0, sprite.x, sprite.y, 0, sprite.scale);

        sprite.x += sprite.dx;
        sprite.y += sprite.dy;

        if (sprite.x + toasterSpriteWidth * sprite.scale < 0) {
            sprite.x = screenW;
            sprite.y = rand() % (screenH - static_cast<int16_t>(toasterSpriteHeight * sprite.scale));
            sprite.dx = (-1 - rand() % 2);
            sprite.dy = (1 + rand() % 2);
        }

        if (sprite.y > screenH) {
            sprite.y = 0;
        }
    }
}

void DisplaySaverScreen::drawNyancatScene() {
    GPGFX_DisplayBase* drv = getRenderer()->getDriver();
    if (drv && drv->isSPI() && screenW == NYANCAT_WIDTH && screenH == NYANCAT_HEIGHT) {
        drv->blitFrame(nyancat_frames[nyancatFrame]);
        nyancatFrame = (nyancatFrame + 1) % NYANCAT_FRAMES;
    }
    getRenderer()->render();
    delay_us(NYANCAT_DELAY_MS * 1000);
}

void DisplaySaverScreen::delay_us(uint32_t us) {
    uint32_t start = time_us_32();
    while ((time_us_32() - start) < us) {
        tight_loop_contents(); // hint for low-power/no-op
    }
}