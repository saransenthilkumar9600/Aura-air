#include "Manual3SchedMode.h"


void Manual3SchedMode::enterMode()
{
    #ifdef LOGGING_MANUAL1MODE
        Log.trace("[Manual3SchedMode::enterMode] - Enter Manual 3 mode");
    #endif

    this->activeNow = true;
    this->timer->reset();
    this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_MANUAL_3_M);
    EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
    Publisher::publishEvent('S', "1.1.22", "support.modes", "Enter Manual 3 mode");
}


void Manual3SchedMode::redefineTimer()
{
    #ifdef LOGGING_MANUAL1MODE
        Log.trace("[Manual3SchedMode::redefineTimer] - Updating the timer period (with the static period)");
    #endif

    this->tmpPeriod = 0.0;

    if (this->mmPtr->getScheduler().pos+1 == 1)
    {
        #ifdef LOGGING_MANUAL1MODE
            Log.trace("[Manual3SchedMode::redefineTimer] - Checking if needed to update the tmp-period in EEPROM");
        #endif
        float tmpTmpPeriod = NULL;
        EepromMngr::get("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", &tmpTmpPeriod);
        if (tmpTmpPeriod > 0.0)
        {
            #ifdef LOGGING_MANUAL1MODE
                Log.info("[Manual3SchedMode::redefineTimer] - Need to update the tmp-period to 0.0 in EEPROM");
            #endif
            EepromMngr::set("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", this->tmpPeriod);
        }
    }

    unsigned int p = int(this->period * 0.5 * 60 * 60 * 1000); // Calculdate the period in millis
    #ifdef LOGGING_MANUAL1MODE
        Log.info("[Manual3SchedMode::redefineTimer] - Calculated static timer period (ms): " + String(p));
    #endif
    this->timer->changePeriod(p);
}


void Manual3SchedMode::exitMode()       
{
    #ifdef LOGGING_MANUAL1MODE
        Log.trace("[Manual3SchedMode::exitMode] - Exit Manual 3 mode");
    #endif

    if (this->timer->isActive())
    {
        #ifdef LOGGING_MANUAL1MODE
            Log.info("[Manual3SchedMode::exitMode] - Need to turn off the Manual3SchedMode timer (scheduler removed)");
        #endif
        this->timer->dispose();
    }

    this->activeNow = false;

    /* If we need to re-define the timer with the actual & real period */
    if (this->tmpPeriod > 0.0)
        this->redefineTimer();

    this->mmPtr->activateDefaultMode();

    Publisher::publishEvent('S', "1.1.23", "support.modes", "Exit Manual 3 mode");
}