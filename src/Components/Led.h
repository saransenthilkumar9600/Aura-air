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
};

#endif