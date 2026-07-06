#include "SilentSchedMode.h"


void SilentSchedMode::enterMode()
{
    #ifdef LOGGING_SILENTMODE
        Log.trace("[SilentSchedMode::enterMode] - Enter Silent mode");
    #endif

    this->activeNow = true;
    this->timer->reset();
    this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_SILENT_M);
    EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
    // if (this->mmPtr->getQuickyMode() == nullptr || !this->mmPtr->getQuickyMode()->activeNow)
    // {
        // this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_SILENT_M);
        // EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
    // }
    Publisher::publishEvent('S', "1.1.12", "support.modes", "Enter silent mode");
}


void SilentSchedMode::redefineTimer()
{
    #ifdef LOGGING_SILENTMODE
        Log.trace("[SilentSchedMode::redefineTimer] - Updating the timer period (with the static period)");
    #endif

    this->tmpPeriod = 0.0;

    if (this->mmPtr->getScheduler().pos+1 == 1)
    {
        #ifdef LOGGING_SILENTMODE
            Log.trace("[SilentSchedMode::redefineTimer] - Checking if needed to update the tmp-period in EEPROM");
        #endif
        float tmpTmpPeriod = NULL;
        EepromMngr::get("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", &tmpTmpPeriod);
        if (tmpTmpPeriod > 0.0)
        {
            #ifdef LOGGING_SILENTMODE
                Log.info("[SilentSchedMode::redefineTimer] - Need to update the tmp-period to 0.0 in EEPROM");
            #endif
            EepromMngr::set("scheduler", "sched" + String(this->mmPtr->getScheduler().pos + 1), "tmp-period", this->tmpPeriod);
        }
    }

    unsigned int p = int(this->period * 0.5 * 60 * 60 * 1000); // Calculdate the period in millis
    #ifdef LOGGING_SILENTMODE
        Log.info("[SilentSchedMode::redefineTimer] - Calculated static timer period (ms): " + String(p));
    #endif
    this->timer->changePeriod(p);
}


void SilentSchedMode::exitMode()
{
    #ifdef LOGGING_SILENTMODE
        Log.trace("[SilentSchedMode::exitMode] - Exit Silent mode");
    #endif

    if (this->timer->isActive())
    {
        #ifdef LOGGING_SILENTMODE
            Log.info("[SilentSchedMode::exitMode] - Need to turn off the SilentSchedMode timer (scheduler removed)");
        #endif
        this->timer->dispose();
    }

    this->activeNow = false;

    /* If we need to re-define the timer with the actual & real period */
    if (this->tmpPeriod > 0.0)
        this->redefineTimer();

    this->mmPtr->getExecComponentsChainCallback()(SysEvent::EXIT_SILENT_M);
    this->mmPtr->activateDefaultMode();

    // if (this->mmPtr->getQuickyMode() == nullptr || !this->mmPtr->getQuickyMode()->activeNow)
    // {
        // this->mmPtr->getExecComponentsChainCallback()(SysEvent::EXIT_SILENT_M);
        // this->mmPtr->activateDefaultMode();
    // }

    Publisher::publishEvent('S', "1.1.13", "support.modes", "Exit silent mode");
}