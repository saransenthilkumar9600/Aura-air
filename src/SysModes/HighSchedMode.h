#ifndef HIGHSCHEDMODE_H
#define HIGHSCHEDMODE_H


#include "ScheduledMode.h"
#include "SysModesMngr.h"


// #define LOGGING_HIGHMODE


class SysModesMngr;


class HighSchedMode : public ScheduledMode
{
public:
    SysModesMngr *mmPtr;

    HighSchedMode() : ScheduledMode(){};
    HighSchedMode(SysMode mode, char *sTime, float period, float tmpPeriod, SysModesMngr *mmPtr) : ScheduledMode(mode, sTime, period, tmpPeriod) { this->mmPtr = mmPtr; }
    void enterMode();
    void exitMode();
    void redefineTimer();
};

#endif