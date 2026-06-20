#include "Scheduler.h"


Scheduler *Scheduler::instance = nullptr;


Scheduler::Scheduler()
{
    this->pos = SchedPosition::FIRST;
    this->size = SchedSize::EMPTY;
    this->schedModeEnds = false;

    for (uint8_t i=0; i<SCHED_POOL_MAX_SIZE; i++)
        this->schedPool[i] = new NightSchedMode();
}


Scheduler& Scheduler::getInstance()
{
    if (instance == nullptr)
        instance = new Scheduler();

    return *instance;
}


void Scheduler::startScheduler()
{
    #ifdef LOGGING_SCHED
        Log.trace("[Scheduler::startScheduler] - Starting the scheduler");
    #endif
    
    if (this->schedPool[this->pos]->tmpPeriod > 0.0)
        this->schedPool[this->pos]->enterMode();
    else
        this->findSchedEntryPoint();
}   


SchedSize Scheduler::stopScheduler()
{
    if (this->schedPool[this->pos]->activeNow)
        this->schedPool[this->pos]->exitMode();

    for (uint8_t i = 0; i < this->size; i++)
        this->schedPool[i] = new NightSchedMode();

    this->pos = SchedPosition::FIRST;
    EepromMngr::set("scheduler", "position", NULL, (uint8_t)this->pos);

    SchedSize tmp = this->size;
    this->size = SchedSize::EMPTY;

    return tmp;
}


void Scheduler::switchPosition()
{
    this->schedModeEnds = false;

    this->schedPool[this->pos]->exitMode();

    if (this->size == SchedSize::ONE)
    {
        #ifdef LOGGING_SCHED
            Log.info("[Scheduler::switchPosition] - The scheduler is in Single Scheduler Mode, no need to switch positions"); 
        #endif
        return;
    }

    this->pos = (SchedPosition)(((uint8_t)this->pos + (uint8_t)1) % (uint8_t)this->size);

    EepromMngr::set("scheduler", "position", NULL, (uint8_t)this->pos);
    #ifdef LOGGING_SCHED
        Log.trace("[Scheduler::switchPosition] - Start looking entry point for mode: " + String(this->schedPool[this->pos]->whoami));
    #endif
}


void Scheduler::findSchedEntryPoint()
{
    if (String(this->schedPool[this->pos]->sTime) == Time.format("%H:%M"))
    {
        #ifdef LOGGING_SCHED
            Log.trace("[Scheduler::findSchedEntryPoint] - Entry point found");
        #endif
        this->schedModeEnds = false;
        this->schedPool[this->pos]->enterMode();
    }
}


void Scheduler::schedModeTimerCallback()
{
    if (this->schedPool[this->pos]->timer->isActive()) 
    {
        #ifdef LOGGING_SCHED
            Log.trace("[Scheduler::schedModeTimerCallback] - The timer of the current scheduled mode (" + String(this->schedPool[this->pos]->whoami) + ") is still active");
        #endif
        return;
    }

    this->schedModeEnds = true;
}