#include "Led.h"
#include <cstring>


Led *Led::instance = nullptr;


Led::Led()
{
    this->coverOpenIndication = LEDStatus(RGB_COLOR_ORANGE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
    this->turnOffLedFlag = false;
    this->inSysMode = false;
    this->constSwitch = SysEvent::LED_ON;
    // Blink machine — safe defaults. Must be explicit: P1 heap objects
    // are NOT zero-initialized. Garbage in blinkActive = false positives.
    this->blinkActive  = false;
    this->blinkPhase   = false;
    this->blinkIsBlue  = true;
    this->blinkLeaveOn = false;
    this->blinkTotal   = 0;
    this->blinkCount   = 0;
    this->lastBlinkMs  = 0;
}


Led& Led::getInstance()
{
    if (instance == nullptr)
        instance = new Led();

    return *instance;
}


void Led::setup()
{
    LEDSystemTheme theme;
    theme.setSignal(LED_SIGNAL_NETWORK_OFF, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);        // Background
    theme.setSignal(LED_SIGNAL_NETWORK_ON, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);         // Background
    theme.setSignal(LED_SIGNAL_NETWORK_CONNECTING, RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL); // Normal
    theme.setSignal(LED_SIGNAL_NETWORK_DHCP, RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL);       // Normal
    theme.setSignal(LED_SIGNAL_NETWORK_CONNECTED, RGB_COLOR_BLUE, LED_PATTERN_FADE, LED_SPEED_NORMAL);   // Background
    theme.setSignal(LED_SIGNAL_CLOUD_CONNECTING, RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_FAST);     // Normal
    theme.setSignal(LED_SIGNAL_CLOUD_HANDSHAKE, RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_FAST);      // Normal
    theme.setSignal(LED_SIGNAL_CLOUD_CONNECTED, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);    // Background
    theme.setSignal(LED_SIGNAL_LISTENING_MODE, RGB_COLOR_GREEN, LED_PATTERN_BLINK, LED_SPEED_SLOW);      // Normal
    theme.apply();
}


void Led::replaceIndication()
{
    LEDSystemTheme theme;
    theme.setSignal(LED_SIGNAL_NETWORK_CONNECTING, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);
    theme.setSignal(LED_SIGNAL_NETWORK_DHCP, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);
    theme.setSignal(LED_SIGNAL_NETWORK_CONNECTED, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);
    theme.setSignal(LED_SIGNAL_CLOUD_CONNECTING, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);
    theme.setSignal(LED_SIGNAL_CLOUD_HANDSHAKE, RGB_COLOR_WHITE, LED_PATTERN_FADE, LED_SPEED_NORMAL);
    theme.apply();
}


void Led::_coverOpenIndication(bool state)
{
    if (state && RGB.controlled())
    {
        // If the cover gets open and LED is under control of other element (like LED switch/Night mode/Silent mode)
        // cancel the control until the cover gets closed
        RGB.control(false);
        this->turnOffLedFlag = true;
    }
    else if (!state && this->turnOffLedFlag)
    {
        RGB.control(true);
        RGB.color(0, 0, 0);
        this->turnOffLedFlag = false;
    }   

    this->coverOpenIndication.setActive(state);
}


void Led::_applyLedPhysicalState(bool on)
{
    if (on)
    {
        if (RGB.controlled() && !inSysMode)
            RGB.control(false);
    }
    else
    {
        if (!RGB.controlled())
        {
            RGB.control(true);
            RGB.color(0, 0, 0);
        }
    }
}

int Led::_parseTimeToMinutes(const char *hhmm)
{
    if (hhmm == nullptr || hhmm[0] == '\0') return -1;
    int h = 0, m = 0;
    if (hhmm[2] != ':' || strlen(hhmm) < 5) return -1;
    h = (hhmm[0] - '0') * 10 + (hhmm[1] - '0');
    m = (hhmm[3] - '0') * 10 + (hhmm[4] - '0');
    if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
    return h * 60 + m;
}

bool Led::_isTimeInRange(const char *current, const char *start, const char *end)
{
    int cur = _parseTimeToMinutes(current);
    int s = _parseTimeToMinutes(start);
    int e = _parseTimeToMinutes(end);
    if (cur < 0 || s < 0 || e < 0) return false;
    if (s <= e)
        return cur >= s && cur < e;
    return cur >= s || cur < e;
}

void Led::applySchedulerCheck(bool force)
{
    static unsigned long lastCheck = 0;
    const unsigned long intervalMs = 10000; // Check every 10 seconds for better responsiveness
    if (!force && millis() - lastCheck < intervalMs && lastCheck != 0)
        return;
    lastCheck = millis();

    if (inSysMode) return;
    // Real fix: applySchedulerCheck is a background maintenance task.
    // It must yield to an active foreground blink sequence.
    // runBlinkTick() will restore LED state correctly when blink ends.
    if (this->blinkActive) return;

    bool enabled = false;
    EepromMngr::get("led_scheduler", "enabled", &enabled);
    if (!enabled)
    {
        bool defaultOn;
        EepromMngr::get("led-switch", "state", &defaultOn);
        _applyLedPhysicalState(defaultOn);
        return;
    }

    char startBuf[6] = {0}, endBuf[6] = {0};
    EepromMngr::get("led_scheduler", "start", NULL, startBuf);
    EepromMngr::get("led_scheduler", "end", NULL, endBuf);
    bool schedState = true;
    EepromMngr::get("led_scheduler", "state", &schedState);

    String currentTime = Time.format("%H:%M");
    bool inRange = _isTimeInRange(currentTime.c_str(), startBuf, endBuf);

    if (inRange)
        _applyLedPhysicalState(schedState);
    else
    {
        bool defaultOn;
        EepromMngr::get("led-switch", "state", &defaultOn);
        _applyLedPhysicalState(defaultOn);
    }
}

int Led::setLedScheduler(String data)
{
    if (data == "disable" || data.length() == 0)
    {
        EepromMngr::set("led_scheduler", "enabled", false);
        applySchedulerCheck(true); // Force immediate check to restore default state
        return 1;
    }

    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);
    if (firstComma < 0 || secondComma < 0 || (size_t)secondComma >= data.length() - 1)
        return 0;

    String startStr = data.substring(0, firstComma);
    String endStr = data.substring(firstComma + 1, secondComma);
    String stateStr = data.substring(secondComma + 1);
    stateStr.trim();
    if (startStr.length() != 5 || endStr.length() != 5) return 0;

    if (_parseTimeToMinutes(startStr.c_str()) < 0 || _parseTimeToMinutes(endStr.c_str()) < 0)
        return 0;

    bool state = (stateStr == "on");

    EepromMngr::set("led_scheduler", "enabled", true);
    EepromMngr::set("led_scheduler", "start", NULL, startStr.c_str());
    EepromMngr::set("led_scheduler", "end", NULL, endStr.c_str());
    EepromMngr::set("led_scheduler", "state", state);

    applySchedulerCheck(true); // Force immediate check to apply new scheduler settings
    return 1;
}

/**
 * This function restored the LED switch state from EEPROM 
 */
void Led::restoreSwitchState()
{
    #ifdef LOGGING_LED
        Log.trace("[Led::restoreSwitchState] - Restore the LED switch state...");
    #endif
    bool switchState;
    EepromMngr::get("led-switch", "state", &switchState);
    if (!switchState)
    {
        #ifdef LOGGING_LED
            Log.trace("[Led::restoreSwitchState] - Turning off the LED");
        #endif
        this->constSwitch = SysEvent::LED_OFF;
        this->handleEvent(SysEvent::LED_OFF);
        return;
    }

    #ifdef LOGGING_LED
        Log.trace("[Led::restoreSwitchState] - Nothing to apply");
    #endif
}


void Led::handleEvent(SysEvent e)
{
    switch(e)
    {
        case SysEvent::COVER_OPEN:
        {
            this->_coverOpenIndication(true);
            break;
        }
        case SysEvent::COVER_CLOSE:
        {
            this->_coverOpenIndication(false);
            break;
        }
        case SysEvent::LED_OFF:
        {
            if (!RGB.controlled())
            {
                RGB.control(true);
                
                RGB.color(0, 0, 0);
            }
            EepromMngr::set("led-switch", "state", false);
            this->constSwitch = e;
            Publisher::publishEvent('S', "1.5.7", "support.led", "LED turned off");
            break;
        }
        case SysEvent::LED_ON:
        {
            if (RGB.controlled() && !inSysMode)
            {
                RGB.control(false);
            }
            EepromMngr::set("led-switch", "state", true);
            this->constSwitch = e;
            Publisher::publishEvent('S', "1.5.6", "support.led", "LED turned on");
            break;
        }
        //case SysEvent::ENTER_SILENT_M:
        case SysEvent::ENTER_NIGHT_M:
        {
            this->inSysMode = true;

            if (!RGB.controlled())
            {
                RGB.control(true);
                RGB.color(0, 0, 0);
            }
            break;
        }
        case SysEvent::EXIT_SILENT_M:
        case SysEvent::EXIT_NIGHT_M:
        case SysEvent::ENTER_AUTO_M:
        case SysEvent::ENTER_HIGH_M:
        case SysEvent::ENTER_LOW_M: /* For quick mode */
        {
            this->inSysMode = false;

    // Real fix: blink owns the LED for its full duration.
    // It IS the visual confirmation of this mode change.
    // When blink ends, runBlinkTick() sets final LED state correctly.
    // Only restore LED state here when no blink is in progress.
    if (!this->blinkActive)
    {
        bool ledSwitch;
        EepromMngr::get("led-switch", "state", &ledSwitch);
        if (ledSwitch && RGB.controlled())
            RGB.control(false);
    }
    break;
        }
        case SysEvent::ENTER_LISTENING_M:
        {
            // Return to the regular pairing LED indication
            this->setup(); 
            // Turn on the LED so the user can understand that the device is in Listening Mode
            if (RGB.controlled())
            {
                RGB.control(false);
            }
            EepromMngr::set("led-switch", "state", true);
            this->constSwitch = e;
            break;
        }
            case SysEvent::BTN_BLINK_1_BLUE:
        this->startBlink(1, true, true);   // 1 blue pulse → LED off
        break;

    case SysEvent::BTN_BLINK_2_BLUE:
        this->startBlink(2, true, true);    // 2 blue pulses → LED on (white)
        break;

    case SysEvent::BTN_BLINK_3_BLUE:
        this->startBlink(3, true, true);    // 3 blue pulses → LED on (white)
        break;

    case SysEvent::BTN_BLINK_4_BLUE:
        this->startBlink(4, true, true);    // 4 blue pulses → LED on (white)
        break;

    case SysEvent::BTN_BLINK_3_ORANGE:
        this->startBlink(3, false, true);  // 3 orange pulses → LED off
        break;

    default:
        break;
    }

    Components::handleEvent(e);
}

// ─────────────────────────────────────────────────────────────
// startBlink() — called once to arm the state machine
// Takes RGB.control(true) immediately for instant first pulse
// ─────────────────────────────────────────────────────────────
void Led::startBlink(uint8_t total, bool isBlue, bool leaveOn)
{
    if (this->blinkActive) return;  // safety: never nest blinks

    this->blinkTotal   = total;
    this->blinkIsBlue  = isBlue;
    this->blinkLeaveOn = leaveOn;
    this->blinkCount   = 0;
    this->blinkPhase   = true;      // begin immediately in ON phase
    this->lastBlinkMs  = millis();
    this->blinkActive  = true;      // arm LAST so guard is valid

    RGB.control(true);
    RGB.color(isBlue ? 0x0000FF : 0xFF5000);
}

// ─────────────────────────────────────────────────────────────
// runBlinkTick() — called every loop() via SysMngr::run()
// 200ms gate: ON phase → OFF phase → ON phase → ... → done
// ─────────────────────────────────────────────────────────────
void Led::runBlinkTick()
{
    if (!this->blinkActive)                        return;
    if (millis() - this->lastBlinkMs < 200)        return;

    this->lastBlinkMs = millis();

    if (this->blinkPhase)
    {
        // ON phase complete → go to OFF
        RGB.color(0, 0, 0);
        
        this->blinkCount++;

        if (this->blinkCount >= this->blinkTotal)
        {
            // All pulses done
            this->blinkActive = false;    // disarm guard FIRST

            if (this->blinkLeaveOn)
                RGB.control(false);       // release → Particle OS white fade ✅
            // else: RGB still controlled + black = LED stays OFF ✅
            this->blinkPhase = false;
        }
    }
    else
    {
        // OFF phase complete → next ON pulse
        this->blinkPhase = true;
        RGB.color(this->blinkIsBlue ? 0x0000FF : 0xFF5000);
    }
}
