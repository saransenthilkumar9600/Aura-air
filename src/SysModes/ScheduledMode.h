#ifndef SCHEDULEDMODE_H
#define SCHEDULEDMODE_H


#include "enums.hpp"
#include "Particle.h"


class ScheduledMode
{
public:
    SysMode whoami;
    Timer *timer;
    char sTime[6];
    float period, tmpPeriod;
    bool activeNow;

    ScheduledMode();
    ScheduledMode(SysMode, char*, float, float);
    virtual void enterMode()=0;
    virtual void exitMode()=0;
};

#endif