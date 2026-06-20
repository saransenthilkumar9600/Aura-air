#ifndef STERIONIZER_H
#define STERIONIZER_H


#include "EepromMngr/EepromMngr.h"
#include "Components.h"
#include "Particle.h"


// #define LOGGING_STERIO
#define HW_VER                 1
#if HW_VER == 1
    #define STERIONIZER_PIN     D2
#endif
#define STERIONIZER_DIAGNOSTIC  A1
#define STERIO_INSPEC_INTERVAL  43200000 //12h


class Sterionizer : public Components
{
private:
    static Sterionizer *instance;
    SysEvent constSwitch;
    bool inHighSysMode, coverOpen, sterioState;
    unsigned long lastSterioInspection;

    Sterionizer();

public:
    static Sterionizer& getInstance();
    static void setup();
    void restoreSwitchState();
    SysEvent getConstSwitch() { return this->constSwitch; }
    bool getSterioState() { return this->sterioState; }
    uint8_t getSterioDiagnos();
    void handleEvent(SysEvent);
};

#endif