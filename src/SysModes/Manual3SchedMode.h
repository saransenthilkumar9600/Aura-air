#ifndef MANUAL3SCHEDMODE_H
#define MANUAL3SCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_MANUAL3MODE


class SysModesMngr;


class Manual3SchedMode : public ScheduledMode
{
public:
    SysModesMngr *mmPtr;

    Manual3SchedMode() : ScheduledMode(){};
    Manual3SchedMode(SysMode mode, char *sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod) { this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};

#endif