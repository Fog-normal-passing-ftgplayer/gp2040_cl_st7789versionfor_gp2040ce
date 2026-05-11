#include "ButtonLayoutScreen.h"
#include "buttonlayouts.h"
#include "drivermanager.h"
#include "drivers/ps4/PS4Driver.h"
#include "drivers/xbone/XBOneDriver.h"
#include "drivers/xinput/XInputDriver.h"
#include "drivers/p5general/P5GeneralDriver.h"

void ButtonLayoutScreen::init() {
    isInputHistoryEnabled = Storage::getInstance().getDisplayOptions().inputHistoryEnabled;
    inputHistoryX = Storage::getInstance().getDisplayOptions().inputHistoryRow;
    inputHistoryY = Storage::getInstance().getDisplayOptions().inputHistoryCol;
    inputHistoryLength = Storage::getInstance().getDisplayOptions().inputHistoryLength;
    bannerDelayStart = getMillis();
    gamepad = Storage::getInstance().GetGamepad();
    inputMode = DriverManager::getInstance().getInputMode();

    EventManager::getInstance().registerEventHandler(GP_EVENT_PROFILE_CHANGE, GPEVENT_CALLBACK(this->handleProfileChange(event)));
    EventManager::getInstance().registerEventHandler(GP_EVENT_USBHOST_MOUNT, GPEVENT_CALLBACK(this->handleUSB(event)));
    EventManager::getInstance().registerEventHandler(GP_EVENT_USBHOST_UNMOUNT, GPEVENT_CALLBACK(this->handleUSB(event)));

    footer = "";
    historyString = "";
    inputHistory.clear();

    uint16_t displayWidth = getRenderer()->getDriver()->getMetrics()->width;
    uint16_t displayHeight = getRenderer()->getDriver()->getMetrics()->height;

    // Scale layout for larger color display (240x135 landscape)
    layoutScaleX = (float)displayWidth / 128.0f * 0.95f;
    layoutScaleY = (float)displayHeight / 64.0f * 0.95f;

    uint16_t layoutW = (uint16_t)(128 * layoutScaleX);
    uint16_t layoutH = (uint16_t)(64 * layoutScaleY);
    _layoutOffsetX = (displayWidth - layoutW) / 2;
    _layoutOffsetY = (displayHeight - layoutH) / 2;
    setViewport(0, 0, displayHeight, displayWidth);

	// load layout (drawElement pushes element to the display list)
    uint16_t elementCtr = 0;
    LayoutManager::LayoutList currLayoutLeft = LayoutManager::getInstance().getLayoutA();
    LayoutManager::LayoutList currLayoutRight = LayoutManager::getInstance().getLayoutB();
    for (elementCtr = 0; elementCtr < currLayoutLeft.size(); elementCtr++) {
        pushElement(currLayoutLeft[elementCtr]);
    }
    for (elementCtr = 0; elementCtr < currLayoutRight.size(); elementCtr++) {
        pushElement(currLayoutRight[elementCtr]);
    }

	// start with profile mode displayed
	bannerDisplay = true;
    prevProfileNumber = -1;

    prevLayoutLeft = Storage::getInstance().getDisplayOptions().buttonLayout;
    prevLayoutRight = Storage::getInstance().getDisplayOptions().buttonLayoutRight;
    prevLeftOptions = Storage::getInstance().getDisplayOptions().buttonLayoutCustomOptions.paramsLeft;
    prevRightOptions = Storage::getInstance().getDisplayOptions().buttonLayoutCustomOptions.paramsRight;
    prevOrientation = Storage::getInstance().getDisplayOptions().buttonLayoutOrientation;

    // we cannot look at macro options enabled, pull the pins
    
    // macro display now uses our pin functions, so we need to check if pins are enabled...
    macroEnabled = false;
    hasTurboAssigned = false;
    // Macro Button initialized by void Gamepad::setup()
    GpioMappingInfo* pinMappings = Storage::getInstance().getProfilePinMappings();
    for (Pin_t pin = 0; pin < (Pin_t)NUM_BANK0_GPIOS; pin++)
    {
        switch( pinMappings[pin].action ) {
            case GpioAction::BUTTON_PRESS_MACRO:
            case GpioAction::BUTTON_PRESS_MACRO_1:
            case GpioAction::BUTTON_PRESS_MACRO_2:
            case GpioAction::BUTTON_PRESS_MACRO_3:
            case GpioAction::BUTTON_PRESS_MACRO_4:
            case GpioAction::BUTTON_PRESS_MACRO_5:
            case GpioAction::BUTTON_PRESS_MACRO_6:
                macroEnabled = true;
                break;
            case GpioAction::BUTTON_PRESS_TURBO:
                hasTurboAssigned = true;
                break;
            default:
                break;
        }
    }

    // determine which fields will be displayed on the status bar
    showInputMode = Storage::getInstance().getDisplayOptions().inputMode;
    showTurboMode = Storage::getInstance().getDisplayOptions().turboMode && hasTurboAssigned;
    showDpadMode = Storage::getInstance().getDisplayOptions().dpadMode;
    showSocdMode = Storage::getInstance().getDisplayOptions().socdMode;
    showMacroMode = Storage::getInstance().getDisplayOptions().macroMode;
    showProfileMode = Storage::getInstance().getDisplayOptions().profileMode;

    getRenderer()->clearScreen();
}

void ButtonLayoutScreen::shutdown() {
    clearElements();

    EventManager::getInstance().unregisterEventHandler(GP_EVENT_PROFILE_CHANGE, GPEVENT_CALLBACK(this->handleProfileChange(event)));
    EventManager::getInstance().unregisterEventHandler(GP_EVENT_USBHOST_MOUNT, GPEVENT_CALLBACK(this->handleUSB(event)));
    EventManager::getInstance().unregisterEventHandler(GP_EVENT_USBHOST_UNMOUNT, GPEVENT_CALLBACK(this->handleUSB(event)));
}

int8_t ButtonLayoutScreen::update() {
    bool configMode = DriverManager::getInstance().isConfigMode();
    uint8_t profileNumber = getGamepad()->getOptions().profileNumber;
    
    // Check if we've updated button layouts while in config mode
    if (configMode) {
        uint8_t layoutLeft = Storage::getInstance().getDisplayOptions().buttonLayout;
        uint8_t layoutRight = Storage::getInstance().getDisplayOptions().buttonLayoutRight;
        uint8_t buttonLayoutOrientation = Storage::getInstance().getDisplayOptions().buttonLayoutOrientation;
        bool inputHistoryEnabled = Storage::getInstance().getDisplayOptions().inputHistoryEnabled;
        if ((prevLayoutLeft != layoutLeft) || (prevLayoutRight != layoutRight) || (isInputHistoryEnabled != inputHistoryEnabled) || compareCustomLayouts() || (prevOrientation != buttonLayoutOrientation)) {
            shutdown();
            init();
        }
    }

    // main logic loop
    if (prevProfileNumber != profileNumber) {
        bannerDelayStart = getMillis();
        prevProfileNumber = profileNumber;
        bannerDisplay = true;
    }

    // main logic loop
	generateHeader();
    if (isInputHistoryEnabled)
		processInputHistory();

    // check for exit/screen change
    if (DriverManager::getInstance().isConfigMode()) {
        uint16_t buttonState = getGamepad()->state.buttons;
        if (prevButtonState && !buttonState) {
            if (prevButtonState == GAMEPAD_MASK_B1) {
                prevButtonState = 0;
                return DisplayMode::CONFIG_INSTRUCTION;
            }
        }
        prevButtonState = buttonState;
    }

	return -1;
}

void ButtonLayoutScreen::generateHeader() {
	// Limit to 21 chars with 6x8 font for now
	statusBar.clear();
	Storage& storage = Storage::getInstance();

	// Display Profile # banner
	if ( bannerDisplay ) {
		if (((getMillis() - bannerDelayStart) / 1000) < bannerDelay) {
			if (bannerMessage.empty()) {
				statusBar.assign(storage.currentProfileLabel(), strlen(storage.currentProfileLabel()));
				if (statusBar.empty()) {
					statusBar = "     Profile #";
					statusBar +=  std::to_string(getGamepad()->getOptions().profileNumber);
				} else {
					statusBar.insert(statusBar.begin(), (21-statusBar.length())/2, ' ');
				}
			} else {
				statusBar = bannerMessage;
			}
			return;
		} else {
			bannerDisplay = false;
            bannerMessage.clear();
		}
	}

    if (showInputMode) {
        // Display standard header
        switch (inputMode)
        {
            case INPUT_MODE_PS3:    statusBar += "PS3"; break;
            case INPUT_MODE_GENERIC: statusBar += "USBHID"; break;
            case INPUT_MODE_SWITCH: statusBar += "SWITCH"; break;
            case INPUT_MODE_MDMINI: statusBar += "GEN/MD"; break;
            case INPUT_MODE_NEOGEO: statusBar += "NGMINI"; break;
            case INPUT_MODE_PCEMINI: statusBar += "PCE/TG"; break;
            case INPUT_MODE_EGRET: statusBar += "EGRET"; break;
            case INPUT_MODE_ASTRO: statusBar += "ASTRO"; break;
            case INPUT_MODE_PSCLASSIC: statusBar += "PSC"; break;
            case INPUT_MODE_XBOXORIGINAL: statusBar += "OGXBOX"; break;
            case INPUT_MODE_SWITCH_PRO: statusBar += "SWPRO"; break;
            case INPUT_MODE_PS4:
                statusBar += "PS4";
                if(((PS4Driver*)DriverManager::getInstance().getDriver())->getAuthSent() == true )
                    statusBar += ":AS";
                else
                    statusBar += "   ";
                break;
            case INPUT_MODE_PS5:
                statusBar += "PS5";
                if(((PS4Driver*)DriverManager::getInstance().getDriver())->getAuthSent() == true )
                    statusBar += ":AS";
                else
                    statusBar += "   ";
                break;
            case INPUT_MODE_P5GENERAL:
                statusBar += "P5G";
                if(((P5GeneralDriver*)DriverManager::getInstance().getDriver())->getAuthSent() == true )
                    statusBar += ":AS";
                else
                    statusBar += "   ";
                break;
            case INPUT_MODE_XBONE:
                statusBar += "XBON";
                if(((XBOneDriver*)DriverManager::getInstance().getDriver())->getAuthSent() == true )
                    statusBar += "E";
                else
                    statusBar += "*";
                break;
            case INPUT_MODE_XINPUT:
                statusBar += "X";
                if(((XInputDriver*)DriverManager::getInstance().getDriver())->getAuthSent() == true )
                    statusBar += "B360";
                else
                    statusBar += "INPUT";
                break;
            case INPUT_MODE_KEYBOARD: statusBar += "HID-KB"; break;
            case INPUT_MODE_CONFIG: statusBar += "CONFIG"; break;
        }
    }

    if (showTurboMode) {
        const TurboOptions& turboOptions = storage.getAddonOptions().turboOptions;
        if ( turboOptions.enabled ) {
            statusBar += " T";
            if ( turboOptions.shotCount < 10 ) // padding
                statusBar += "0";
            statusBar += std::to_string(turboOptions.shotCount);
        } else {
            statusBar += "    "; // no turbo, don't show Txx setting
        }
    }

	const GamepadOptions & options = gamepad->getOptions();

    if (showDpadMode) {
        switch (gamepad->getActiveDpadMode())
        {
            case DPAD_MODE_DIGITAL:      statusBar += " D"; break;
            case DPAD_MODE_LEFT_ANALOG:  statusBar += " L"; break;
            case DPAD_MODE_RIGHT_ANALOG: statusBar += " R"; break;
        }
    }

    if (showSocdMode) {
        switch (Gamepad::resolveSOCDMode(options))
        {
            case SOCD_MODE_NEUTRAL:               statusBar += " SOCD-N"; break;
            case SOCD_MODE_UP_PRIORITY:           statusBar += " SOCD-U"; break;
            case SOCD_MODE_SECOND_INPUT_PRIORITY: statusBar += " SOCD-L"; break;
            case SOCD_MODE_FIRST_INPUT_PRIORITY:  statusBar += " SOCD-F"; break;
            case SOCD_MODE_BYPASS:                statusBar += " SOCD-X"; break;
        }
    }

    if (showMacroMode && macroEnabled) statusBar += " M";

    if (showProfileMode) {
        statusBar += " ";

        std::string profile;
        profile.assign(storage.currentProfileLabel(), strlen(storage.currentProfileLabel()));
        if (profile.empty()) {
            statusBar += std::to_string(options.profileNumber);
        } else {
            statusBar += profile;
        }
    }

    trim(statusBar);
}

void ButtonLayoutScreen::drawScreen() {
    uint16_t displayWidth = getRenderer()->getDriver()->getMetrics()->width;
    uint16_t displayHeight = getRenderer()->getDriver()->getMetrics()->height;
    uint16_t barWidth = (displayWidth > 128) ? displayWidth - 1 : 127;
    uint8_t footerRow = (displayHeight > 64) ? (displayHeight / 8) - 2 : 6;
    if (footerRow < 6) footerRow = 6;
    // Center status bar text horizontally on wider displays
    uint8_t fontW = 6;
    uint8_t textGridX = (displayWidth > (uint16_t)(statusBar.size() * fontW))
        ? (displayWidth - (uint16_t)(statusBar.size() * fontW)) / (fontW * 2) : 0;

    // Use configured status bar text color (only for SPI color display)
    uint16_t barColor = Storage::getInstance().getDisplayOptions().spiStatusBarColor;
    GPGFX_DisplayBase* drv = getRenderer()->getDriver();
    drv->setOverrideColor(barColor);

    // Always draw status bar with solid background for visibility on color displays
    if (bannerDisplay) {
        getRenderer()->drawRectangle(0, 0, barWidth, 7, true, true);
    	getRenderer()->drawText(textGridX, 0, statusBar, true);
    } else {
        // Draw dark background behind status bar text for contrast
        getRenderer()->drawRectangle(0, 0, barWidth, 7, false, true);
        getRenderer()->drawText(textGridX, 0, statusBar, false);
    }
    drv->setOverrideColor(0);

    getRenderer()->drawText(0, footerRow, footer);

    // Gothic medieval corner ornaments at screen edges
    uint32_t gold = 0xFDA0, dark = 0x8400;
    int W = displayWidth, H = displayHeight, S = 24;
    auto drawCorner = [&](int ox, int oy, int mx, int my) {
        auto px = [&](int x) { return ox + (mx ? -(x) : x); };
        auto py = [&](int y) { return oy + (my ? -(y) : y); };
        auto dp = [&](int x, int y, uint32_t c) { getRenderer()->drawPixel(px(x), py(y), c); };
        // L-bracket along edges
        for (int i = 0; i <= S; i++) { dp(i, 0, gold); dp(0, i, gold); }
        // Gothic pointed arch
        for (int i = 3; i <= S-3; i++) {
            int y = S - i;
            dp(i, y, gold);
            if (i < S-4) { dp(i-1, y, dark); dp(i, y-1, dark); }
        }
        // Inner shadow arch
        for (int i = 6; i <= S-6; i++) {
            int y = S - i - 3;
            if (y > 0) { dp(i, y, dark); if (i%3==0) dp(i+1, y-1, gold); }
        }
        // Ornamental dots along diagonal
        for (int d = 0; d < 4; d++) {
            int x = 4 + d*5, y = S - 4 - d*5;
            if (y > 0) { dp(x, y, gold); dp(x+1, y-1, dark); }
        }
        // Edge post circles
        dp(10, 1, dark); dp(1, 10, dark);
        // Diamond tip
        int dx = S-8, dy = S-8;
        dp(dx, dy, gold); dp(dx+1, dy, dark); dp(dx-1, dy, dark);
        dp(dx, dy+1, dark); dp(dx, dy-1, dark);
    };
    drawCorner(0, 0, 0, 0);       // top-left
    drawCorner(W-1, 0, 1, 0);     // top-right
    drawCorner(0, H-1, 0, 1);     // bottom-left
    drawCorner(W-1, H-1, 1, 1);   // bottom-right
}

GPLever* ButtonLayoutScreen::addLever(uint16_t startX, uint16_t startY, uint16_t sizeX, uint16_t sizeY, uint16_t strokeColor, uint16_t fillColor, uint16_t inputType) {
    GPLever* lever = new GPLever();
    lever->setRenderer(getRenderer());
    lever->setPosition((uint16_t)(startX * layoutScaleX) + _layoutOffsetX, (uint16_t)(startY * layoutScaleY) + _layoutOffsetY);
    lever->setRadius((uint16_t)(sizeX * ((layoutScaleX + layoutScaleY) / 2.0f)));
    lever->setStrokeColor(strokeColor);
    lever->setFillColor(fillColor);
    lever->setInputType(inputType);
    lever->setViewport(this->getViewport());
    return (GPLever*)addElement(lever);
}

GPButton* ButtonLayoutScreen::addButton(uint16_t startX, uint16_t startY, uint16_t sizeX, uint16_t sizeY, uint16_t strokeColor, uint16_t fillColor, int16_t inputMask) {
    GPButton* button = new GPButton();
    button->setRenderer(getRenderer());
    button->setPosition((uint16_t)(startX * layoutScaleX) + _layoutOffsetX, (uint16_t)(startY * layoutScaleY) + _layoutOffsetY);
    button->setStrokeColor(strokeColor);
    button->setFillColor(fillColor);
    button->setSize((uint16_t)(sizeX * layoutScaleX), (uint16_t)(sizeY * layoutScaleY));
    button->setInputMask(inputMask);
    button->setViewport(this->getViewport());
    return (GPButton*)addElement(button);
}

GPShape* ButtonLayoutScreen::addShape(uint16_t startX, uint16_t startY, uint16_t sizeX, uint16_t sizeY, uint16_t strokeColor, uint16_t fillColor) {
    GPShape* shape = new GPShape();
    shape->setRenderer(getRenderer());
    shape->setPosition((uint16_t)(startX * layoutScaleX) + _layoutOffsetX, (uint16_t)(startY * layoutScaleY) + _layoutOffsetY);
    shape->setStrokeColor(strokeColor);
    shape->setFillColor(fillColor);
    shape->setSize((uint16_t)(sizeX * layoutScaleX), (uint16_t)(sizeY * layoutScaleY));
    shape->setViewport(this->getViewport());
    return (GPShape*)addElement(shape);
}

GPSprite* ButtonLayoutScreen::addSprite(uint16_t startX, uint16_t startY, uint16_t sizeX, uint16_t sizeY) {
    GPSprite* sprite = new GPSprite();
    sprite->setRenderer(getRenderer());
    sprite->setPosition((uint16_t)(startX * layoutScaleX) + _layoutOffsetX, (uint16_t)(startY * layoutScaleY) + _layoutOffsetY);
    sprite->setSize((uint16_t)(sizeX * layoutScaleX), (uint16_t)(sizeY * layoutScaleY));
    sprite->setViewport(this->getViewport());
    return (GPSprite*)addElement(sprite);
}

GPWidget* ButtonLayoutScreen::pushElement(GPButtonLayout element) {
    if (element.elementType == GP_ELEMENT_LEVER) {
        return addLever(element.parameters.x1, element.parameters.y1, element.parameters.x2, element.parameters.y2, element.parameters.stroke, element.parameters.fill, element.parameters.value);
    } else if ((element.elementType == GP_ELEMENT_BTN_BUTTON) || (element.elementType == GP_ELEMENT_DIR_BUTTON) || (element.elementType == GP_ELEMENT_PIN_BUTTON)) {
        GPButton* button = addButton(element.parameters.x1, element.parameters.y1, element.parameters.x2, element.parameters.y2, element.parameters.stroke, element.parameters.fill, element.parameters.value);

        // set type of button
        button->setInputType(element.elementType);
        button->setInputDirection(false);
        button->setShape((GPShape_Type)element.parameters.shape);
        button->setAngle(element.parameters.angleStart);
        button->setAngleEnd(element.parameters.angleEnd);
        button->setClosed(element.parameters.closed);

        if (element.elementType == GP_ELEMENT_DIR_BUTTON) button->setInputDirection(true);

        return (GPWidget*)button;
    } else if (element.elementType == GP_ELEMENT_SPRITE) {
        return addSprite(element.parameters.x1, element.parameters.y1, element.parameters.x2, element.parameters.y2);
    } else if (element.elementType == GP_ELEMENT_SHAPE) {
        GPShape* shape = addShape(element.parameters.x1, element.parameters.y1, element.parameters.x2, element.parameters.y2, element.parameters.stroke, element.parameters.fill);
        shape->setShape((GPShape_Type)element.parameters.shape);
        shape->setAngle(element.parameters.angleStart);
        shape->setAngleEnd(element.parameters.angleEnd);
        shape->setClosed(element.parameters.closed);
        return shape;
    }
    return NULL;
}

void ButtonLayoutScreen::processInputHistory() {
	std::deque<std::string> pressed;

	// Get key states
	std::array<bool, INPUT_HISTORY_MAX_INPUTS> currentInput = {

		pressedUp(),
		pressedDown(),
		pressedLeft(),
		pressedRight(),

		pressedUpLeft(),
		pressedUpRight(),
		pressedDownLeft(),
		pressedDownRight(),

		getProcessedGamepad()->pressedB1(),
		getProcessedGamepad()->pressedB2(),
		getProcessedGamepad()->pressedB3(),
		getProcessedGamepad()->pressedB4(),
		getProcessedGamepad()->pressedL1(),
		getProcessedGamepad()->pressedR1(),
		getProcessedGamepad()->pressedL2(),
		getProcessedGamepad()->pressedR2(),
		getProcessedGamepad()->pressedS1(),
		getProcessedGamepad()->pressedS2(),
		getProcessedGamepad()->pressedL3(),
		getProcessedGamepad()->pressedR3(),
		getProcessedGamepad()->pressedA1(),
		getProcessedGamepad()->pressedA2(),
	};

	uint8_t mode = ((displayModeLookup.count(inputMode) > 0) ? displayModeLookup.at(inputMode) : 0);

	// Check if any new keys have been pressed
	if (lastInput != currentInput) {
		// Iterate through array
		for (uint8_t x=0; x<INPUT_HISTORY_MAX_INPUTS; x++) {
			// Add any pressed keys to deque
			std::string inputChar(displayNames[mode][x]);
			if (currentInput[x] && (inputChar != "")) pressed.push_back(inputChar);
		}
		// Update the last keypress array
		lastInput = currentInput;
	}

	if (pressed.size() > 0) {
		std::string newInput;
		for(const auto &s : pressed) {
				if(!newInput.empty())
						newInput += "+";
				newInput += s;
		}

		inputHistory.push_back(newInput);
	}

	if (inputHistory.size() > (inputHistoryLength / 2) + 1) {
		inputHistory.pop_front();
	}

	std::string ret;

	for (auto it = inputHistory.crbegin(); it != inputHistory.crend(); ++it) {
		std::string newRet = ret;
		if (!newRet.empty())
			newRet = " " + newRet;

		newRet = *it + newRet;
		ret = newRet;

		if (ret.size() >= inputHistoryLength) {
			break;
		}
	}

	if(ret.size() >= inputHistoryLength) {
		historyString = ret.substr(ret.size() - inputHistoryLength);
	} else {
		historyString = ret;
	}

    footer = historyString;
}

bool ButtonLayoutScreen::compareCustomLayouts()
{
    ButtonLayoutParamsLeft leftOptions = Storage::getInstance().getDisplayOptions().buttonLayoutCustomOptions.paramsLeft;
    ButtonLayoutParamsRight rightOptions = Storage::getInstance().getDisplayOptions().buttonLayoutCustomOptions.paramsRight;

    bool leftChanged = ((leftOptions.layout != prevLeftOptions.layout) || (leftOptions.common.startX != prevLeftOptions.common.startX) || (leftOptions.common.startY != prevLeftOptions.common.startY) || (leftOptions.common.buttonPadding != prevLeftOptions.common.buttonPadding) || (leftOptions.common.buttonRadius != prevLeftOptions.common.buttonRadius));
    bool rightChanged = ((rightOptions.layout != prevRightOptions.layout) || (rightOptions.common.startX != prevRightOptions.common.startX) || (rightOptions.common.startY != prevRightOptions.common.startY) || (rightOptions.common.buttonPadding != prevRightOptions.common.buttonPadding) || (rightOptions.common.buttonRadius != prevRightOptions.common.buttonRadius));
    
    return (leftChanged || rightChanged);
}

bool ButtonLayoutScreen::pressedUp()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == GAMEPAD_MASK_UP);
        case DPAD_MODE_LEFT_ANALOG:  return getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MIN;
        case DPAD_MODE_RIGHT_ANALOG: return getProcessedGamepad()->state.ry == GAMEPAD_JOYSTICK_MIN;
    }

    return false;
}

bool ButtonLayoutScreen::pressedDown()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == GAMEPAD_MASK_DOWN);
        case DPAD_MODE_LEFT_ANALOG:  return getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MAX;
        case DPAD_MODE_RIGHT_ANALOG: return getProcessedGamepad()->state.ry == GAMEPAD_JOYSTICK_MAX;
    }

    return false;
}

bool ButtonLayoutScreen::pressedLeft()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == GAMEPAD_MASK_LEFT);
        case DPAD_MODE_LEFT_ANALOG:  return getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MIN;
        case DPAD_MODE_RIGHT_ANALOG: return getProcessedGamepad()->state.rx == GAMEPAD_JOYSTICK_MIN;
    }

    return false;
}

bool ButtonLayoutScreen::pressedRight()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == GAMEPAD_MASK_RIGHT);
        case DPAD_MODE_LEFT_ANALOG:  return getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MAX;
        case DPAD_MODE_RIGHT_ANALOG: return getProcessedGamepad()->state.rx == GAMEPAD_JOYSTICK_MAX;
    }

    return false;
}

bool ButtonLayoutScreen::pressedUpLeft()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == (GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT));
        case DPAD_MODE_LEFT_ANALOG:  return (getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MIN) && (getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MIN);
        case DPAD_MODE_RIGHT_ANALOG: return (getProcessedGamepad()->state.rx == GAMEPAD_JOYSTICK_MIN) && (getProcessedGamepad()->state.ry == GAMEPAD_JOYSTICK_MIN);
    }

    return false;
}

bool ButtonLayoutScreen::pressedUpRight()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == (GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT));
        case DPAD_MODE_LEFT_ANALOG:  return (getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MAX) && (getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MIN);
        case DPAD_MODE_RIGHT_ANALOG: return (getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MAX) && (getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MIN);
    }

    return false;
}

bool ButtonLayoutScreen::pressedDownLeft()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == (GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT));
        case DPAD_MODE_LEFT_ANALOG:  return (getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MIN) && (getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MAX);
        case DPAD_MODE_RIGHT_ANALOG: return (getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MIN) && (getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MAX);
    }

    return false;
}

bool ButtonLayoutScreen::pressedDownRight()
{
    switch (getGamepad()->getActiveDpadMode())
    {
        case DPAD_MODE_DIGITAL:      return ((getProcessedGamepad()->state.dpad & GAMEPAD_MASK_DPAD) == (GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT));
        case DPAD_MODE_LEFT_ANALOG:  return (getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MAX) && (getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MAX);
        case DPAD_MODE_RIGHT_ANALOG: return (getProcessedGamepad()->state.lx == GAMEPAD_JOYSTICK_MAX) && (getProcessedGamepad()->state.ly == GAMEPAD_JOYSTICK_MAX);
    }

    return false;
}

void ButtonLayoutScreen::handleProfileChange(GPEvent* e) {
    GPProfileChangeEvent* event = (GPProfileChangeEvent*)e;

    profileNumber = event->currentValue;
    prevProfileNumber = event->previousValue;
}

void ButtonLayoutScreen::handleUSB(GPEvent* e) {
    bannerDelayStart = getMillis();
    prevProfileNumber = profileNumber;

    if (e->eventType() == GP_EVENT_USBHOST_MOUNT) {
        bannerMessage = "    USB Connected";
    } else if (e->eventType() == GP_EVENT_USBHOST_UNMOUNT) {
        bannerMessage = "  USB Disconnnected";
    }
    bannerDisplay = true;
}

void ButtonLayoutScreen::trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            [](unsigned char c) { return !std::isspace(c); }));
}
