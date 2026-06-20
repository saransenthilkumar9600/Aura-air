#ifndef MANUAL2SCHEDMODE_H
#define MANUAL2SCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_MANUAL2MODE


class SysModesMngr;


class Manual2SchedMode : public ScheduledMode
{
public:
    SysModesMngr *mmPtr;

    Manual2SchedMode() : ScheduledMode(){};
    Manual2SchedMode(SysMode mode, char *sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod) { this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};

#endif