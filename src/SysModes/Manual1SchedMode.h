#ifndef MANUAL1SCHEDMODE_H
#define MANUAL1SCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_MANUAL1MODE


class SysModesMngr;


class Manual1SchedMode : public ScheduledMode
{
public:
    SysModesMngr *mmPtr;

    Manual1SchedMode() : ScheduledMode(){};
    Manual1SchedMode(SysMode mode, char *sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod) { this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};

#endif