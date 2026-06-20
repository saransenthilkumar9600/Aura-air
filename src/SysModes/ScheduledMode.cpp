#include "ScheduledMode.h"


ScheduledMode::ScheduledMode()
{
    this->whoami = SysMode::NOT_INIT;
    this->period = 0.0;
    this->tmpPeriod = 0.0;
    this->activeNow = false;
    this->timer = nullptr;
}

ScheduledMode::ScheduledMode(SysMode mode, char *sTime, float period, float tmpPeriod)
{
    this->whoami = mode;
    strncpy(this->sTime, sTime, 6);
    this->tmpPeriod = tmpPeriod;
    this->period = period;
    this->activeNow = false;
}