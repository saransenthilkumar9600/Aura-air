#ifndef LED_H
#define LED_H


#include "EepromMngr/EepromMngr.h"
#include "Components.h"
#include "Particle.h"


// #define LOGGING_LED


class Led : public Components
{
private:
    static Led *instance;
    LEDStatus coverOpenIndication;
    bool turnOffLedFlag, inSysMode;
    SysEvent constSwitch;
    // Blink state machine — all driven by runBlinkTick() in loop
    bool          blinkActive;   // true = a blink sequence owns the LED
    bool          blinkPhase;    // true = currently in ON phase (showing colour)
    bool          blinkIsBlue;   // true = blue colour, false = orange
    bool          blinkLeaveOn;  // true = release to Particle OS after blink
    uint8_t       blinkTotal;    // total ON pulses to deliver
    uint8_t       blinkCount;    // ON pulses completed so far
    unsigned long lastBlinkMs;   // millis() timestamp of last phase flip

    Led();
    void _coverOpenIndication(bool);
    void _applyLedPhysicalState(bool on);
    static int _parseTimeToMinutes(const char *hhmm);
    static bool _isTimeInRange(const char *current, const char *start, const char *end);

public:
    static Led& getInstance();
    static void setup();
    void replaceIndication();
    void restoreSwitchState();
    void applySchedulerCheck(bool force = false);
    SysEvent getSwitchState() { return this->constSwitch; }
    void handleEvent(SysEvent);
    /** @param data "HH:mm,HH:mm,on" or "HH:mm,HH:mm,off" or "disable" to disable scheduler. Returns 1 on success, 0 on failure. */
    int setLedScheduler(String data);
    // Button blink public API
    void startBlink(uint8_t total, bool isBlue, bool leaveOn);
    void runBlinkTick();
    bool isBlinking() { return this->blinkActive; }
    bool isBlinkActive() const { return this->blinkActive; } 
};

#endif