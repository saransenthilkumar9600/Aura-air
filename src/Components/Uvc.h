#ifndef UVC_H
#define UVC_H


#include "EepromMngr/EepromMngr.h"
#include "Components.h"
#include "Particle.h"


// #define LOGGING_UVC
#define HW_VER 1
#if HW_VER == 1
    #define UVC_PIN P1S1
#endif


class Uvc : public Components
{
private:
    static Uvc *instance;
    SysEvent constSwitch;
    bool inHighSysMode, coverOpen, uvcState;

    Uvc();

public:
    static Uvc& getInstance();
    static void setup();
    void restoreSwitchState();
    SysEvent getConstSwitch() { return this->constSwitch; }
    bool getUvcState() { return this->uvcState; }
    void handleEvent(SysEvent);
};

#endif