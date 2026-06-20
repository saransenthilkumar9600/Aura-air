#ifndef NIGHTSCHEDMODE_H
#define NIGHTSCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_NIGHTMODE


class SysModesMngr;


class NightSchedMode : public ScheduledMode 
{
private:
    SysModesMngr *mmPtr;

public:
    NightSchedMode() : ScheduledMode(){};
    NightSchedMode(SysMode mode, char* sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod){ this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};

#endif