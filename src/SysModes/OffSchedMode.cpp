#include "OffSchedMode.h"


void OffSchedMode::enterMode()
{
    #ifdef LOGGING_LOWMODE
        Log.trace("[OffSchedMode::enterMode] - Enter Low mode");
    #endif

    this->activeNow = true;
    this->timer->reset();
    this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_OFF_M);
    EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
    // if (this->mmPtr->getQuickyMode() == nullptr || !this->mmPtr->getQuickyMode()->activeNow)
    // {
        // this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_OFF_M);
        // EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
    // }
    Publisher::publishEvent('S', "1.1.16", "support.modes", "Enter off mode");
}


void OffSchedMode::redefineTimer()
{
    #ifdef LOGGING_LOWMODE
        Log.trace("[OffSchedMode::redefineTimer] - Updating the timer period (with the static period)");
    #endif

    this->tmpPeriod = 0.0;

    if (this->mmPtr->getScheduler().pos+1 == 1)
    {
        #ifdef LOGGING_LOWMODE
            Log.trace("[OffSchedMode::redefineTimer] - Checking if needed to update the tmp-period in EEPROM");
        #endif
        float tmpTmpPeriod = NULL;
        EepromMngr::get("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", &tmpTmpPeriod);
        if (tmpTmpPeriod > 0.0)
        {
            #ifdef LOGGING_LOWMODE
                Log.info("[OffSchedMode::redefineTimer] - Need to update the tmp-period to 0.0 in EEPROM");
            #endif
            EepromMngr::set("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", this->tmpPeriod);
        }
    }

    unsigned int p = int(this->period * 0.5 * 60 * 60 * 1000); // Calculdate the period in millis
    #ifdef LOGGING_LOWMODE
        Log.info("[OffSchedMode::redefineTimer] - Calculated static timer period (ms): " + String(p));
    #endif
    this->timer->changePeriod(p);
}


void OffSchedMode::exitMode()       
{
    #ifdef LOGGING_LOWMODE
        Log.trace("[OffSchedMode::exitMode] - Exit Low mode");
    #endif

    if (this->timer->isActive())
    {
        #ifdef LOGGING_LOWMODE
            Log.info("[OffSchedMode::exitMode] - Need to turn off the OffSchedMode timer (scheduler removed)");
        #endif
        this->timer->dispose();
    }

    this->activeNow = false;

    /* If we need to re-define the timer with the actual & real period */
    if (this->tmpPeriod > 0.0)
        this->redefineTimer();

    this->mmPtr->activateDefaultMode();

    Publisher::publishEvent('S', "1.1.17", "support.modes", "Exit off mode");
}