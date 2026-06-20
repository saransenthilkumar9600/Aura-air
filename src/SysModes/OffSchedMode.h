#ifndef OFFSCHEDMODE_H
#define OFFSCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_LOWMODE


class SysModesMngr;


class OffSchedMode : public ScheduledMode
{
public:
    SysModesMngr *mmPtr;

    OffSchedMode() : ScheduledMode(){};
    OffSchedMode(SysMode mode, char *sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod) { this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};

#endif