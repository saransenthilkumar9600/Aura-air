#ifndef SCHEDULER_H
#define SCHEDULER_H


#include "SilentSchedMode.h"
#include "NightSchedMode.h"
#include "HighSchedMode.h"
#include "LowSchedMode.h"
#include "OffSchedMode.h"
#include "Manual1SchedMode.h"
#include "Manual2SchedMode.h"
#include "Manual3SchedMode.h"
#include "enums.hpp"


// #define LOGGING_SCHED
#define SCHED_POOL_MAX_SIZE 6


class Scheduler
{
private:
    static Scheduler *instance;
    
    Scheduler();

public:
    SchedSize size;
    SchedPosition pos;
    ScheduledMode *schedPool[SCHED_POOL_MAX_SIZE];
    bool schedModeEnds;

    static Scheduler &getInstance();
    void startScheduler();
    SchedSize stopScheduler();
    void stopScheduler(SchedPosition, SysMode);
    void switchPosition();
    void findSchedEntryPoint();
    void schedModeTimerCallback();
};

#endif