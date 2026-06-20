#ifndef SILENTSCHEDMODE_H
#define SILENTSCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_SILENTMODE


class SysModesMngr;


class SilentSchedMode : public ScheduledMode
{
public:
    SysModesMngr *mmPtr;

    SilentSchedMode() : ScheduledMode(){};
    SilentSchedMode(SysMode mode, char *sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod) { this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};
#endif