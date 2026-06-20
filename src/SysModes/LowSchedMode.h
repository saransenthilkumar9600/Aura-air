#ifndef LOWSCHEDMODE_H
#define LOWSCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_LOWMODE


class SysModesMngr;


class LowSchedMode : public ScheduledMode
{
public:
    SysModesMngr *mmPtr;

    LowSchedMode() : ScheduledMode(){};
    LowSchedMode(SysMode mode, char *sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod) { this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};

#endif