#include "NightSchedMode.h"


void NightSchedMode::enterMode()
{
    #ifdef LOGGING_NIGHTMODE
        Log.trace("[NightSchedMode::enterMode] - Enter Night mode");
    #endif

    this->activeNow = true;
    this->timer->reset();
    this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_NIGHT_M);
    EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
    // if (this->mmPtr->getQuickyMode() == nullptr || !this->mmPtr->getQuickyMode()->activeNow)
    // {
        // this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_NIGHT_M);
        // EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
    // }
    Publisher::publishEvent('S', "1.1.8", "support.modes", "Enter night mode");
}


void NightSchedMode::redefineTimer()
{
    #ifdef LOGGING_NIGHTMODE
        Log.trace("[NightSchedMode::redefineTimer] - Updating the timer period (with the static period)");
    #endif

    this->tmpPeriod = 0.0;

    if (this->mmPtr->getScheduler().pos+1 == 1)
    {
        #ifdef LOGGING_NIGHTMODE
            Log.trace("[NightSchedMode::redefineTimer] - Checking if needed to update the tmp-period in EEPROM");
        #endif
        float tmpTmpPeriod = 0;
        EepromMngr::get("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", &tmpTmpPeriod);
        if (tmpTmpPeriod > 0.0)
        {
            #ifdef LOGGING_NIGHTMODE
                Log.info("[NightSchedMode::redefineTimer] - Need to update the tmp-period to 0.0 in EEPROM");
            #endif
            EepromMngr::set("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", this->tmpPeriod);
        }
    }

    unsigned int p = int(this->period * 0.5 * 60 * 60 * 1000); // Calculdate the period in millis
    #ifdef LOGGING_NIGHTMODE
        Log.info("[NightSchedMode::redefineTimer] - Calculated static timer period (ms): " + String(p));
    #endif
    this->timer->changePeriod(p);
}


void NightSchedMode::exitMode()
{
    #ifdef LOGGING_NIGHTMODE
        Log.trace("[NightSchedMode::exitMode] - Exit Night mode");
    #endif

    if (this->timer->isActive())
    {
        #ifdef LOGGING_NIGHTMODE
            Log.info("[NightSchedMode::exitMode] - Need to turn off the NightSchedMode timer (scheduler removed)");
        #endif
        this->timer->dispose();
    }

    this->activeNow = false;

    /* If we need to re-define the timer with the actual & real period */
    if (this->tmpPeriod > 0.0) 
        this->redefineTimer();

    this->mmPtr->getExecComponentsChainCallback()(SysEvent::EXIT_NIGHT_M);
    this->mmPtr->activateDefaultMode();

    // if (this->mmPtr->getQuickyMode() == nullptr || !this->mmPtr->getQuickyMode()->activeNow)
    // {
    //     this->mmPtr->getExecComponentsChainCallback()(SysEvent::EXIT_NIGHT_M);
    //     this->mmPtr->activateDefaultMode();
    // }

    Publisher::publishEvent('S', "1.1.9", "support.modes", "Exit night mode");
}