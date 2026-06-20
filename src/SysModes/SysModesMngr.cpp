#include "SysModesMngr.h"


SysModesMngr *SysModesMngr::instance = nullptr;


SysModesMngr::SysModesMngr() : sched(Scheduler::getInstance())
{
    this->defaultMode = SysMode::LOW_M;
    
    // this->quicky = nullptr;
    // this->quickModeEnds = false;
}


SysModesMngr& SysModesMngr::getInstance()
{
    if (instance == nullptr)
        instance = new SysModesMngr();

    return *instance;
}


void SysModesMngr::setup(callbackFunc execComponentsChainCallback)
{
    this->execComponentsChainCallback = execComponentsChainCallback;
    this->activateDefaultMode();
}


void SysModesMngr::restoreScheduler()
{
    // Check if there is a scheduler to restore based on the scheduler size stored in EEPROM
    uint8_t tmpSchedSize;
    EepromMngr::get("scheduler", "size", NULL, &tmpSchedSize);
    if ((SchedSize)tmpSchedSize == SchedSize::EMPTY) return;
    
    this->sched.size = (SchedSize)tmpSchedSize;

    // Restored the last position of the scheduler before restart happened
    uint8_t tmpPos;
    EepromMngr::get("scheduler", "position", NULL, &tmpPos);
    #ifdef LOGGING_SYSMODES
        Log.info("[SysModesMngr::restoreScheduler] - restored position is: " + String(tmpPos));
    #endif
    this->sched.pos = (SchedPosition)tmpPos;

    // For each scheduled mode in the scheduler, restored the information from EEPROM
    for (uint8_t i=0; i<this->sched.size; i++)
    {
        int8_t tmpWahoami;
        EepromMngr::get("scheduler", "sched" + String(i+1), "whoami", &tmpWahoami);

        char tmpSTime[6];
        EepromMngr::get("scheduler", "sched" + String(i+1), "start-time", (char*)&tmpSTime);
        const char *sTime = tmpSTime;

        float period;
        EepromMngr::get("scheduler", "sched" + String(i+1), "period", &period);

        float tmpPeriod = 0.0;

        if (i == this->sched.pos)
        {
            #ifdef LOGGING_SYSMODES
                Log.trace("[SysModesMngr::restoreScheduler] - Checking whether it is required to return to a scheduled mode");
            #endif

            String currDayStr = String(Time.day()); const char *day = currDayStr.c_str();
            String currMonthStr = String(Time.month()); const char *month = currMonthStr.c_str();
            String currYearStr = String(Time.year()); const char *year = currYearStr.c_str();
            char *fullSTimeDesc = new char[strlen(day) + strlen(" ") + strlen(month) + strlen(" ") + strlen(year) + strlen(" ") + strlen(sTime) + strlen(":00")];
            strcpy(fullSTimeDesc, day);strcat(fullSTimeDesc, " ");strcat(fullSTimeDesc, month);strcat(fullSTimeDesc, " ");strcat(fullSTimeDesc, year);strcat(fullSTimeDesc, " ");strcat(fullSTimeDesc, sTime);strcat(fullSTimeDesc, ":00");

            struct tm tmpTm;
            strptime(fullSTimeDesc, "%d %m %Y %H:%M:%S", &tmpTm);

            time_t sTime_t = mktime(&tmpTm);
            time_t currTime_t = Time.local();

            #ifdef LOGGING_SYSMODES
                Log.info("[SysModesMngr::restoreScheduler] - Scheduled mode START time: " + String(ctime(&sTime_t)));
                Log.info("[SysModesMngr::restoreScheduler] - Scheduled mode CURRENT time: " + String(ctime(&currTime_t)));
            #endif

            double diff = (difftime(currTime_t, sTime_t)/3600);
            #ifdef LOGGING_SYSMODES
                Log.info("[SysModesMngr::restoreScheduler] - Calculated different time in hours (current - start time): " + String(diff));
            #endif

            if (diff < 0)
            {
                #ifdef LOGGING_SYSMODES
                    Log.info("[SysModesMngr::restoreScheduler] - The calculated different time is smaller than 0. Recalculate the different now");
                #endif

                String currDayStr = String(Time.day()-1); day = currDayStr.c_str();
                fullSTimeDesc = new char[strlen(day) + strlen(" ") + strlen(month) + strlen(" ") + strlen(year) + strlen(" ") + strlen(sTime) + strlen(":00")];
                strcpy(fullSTimeDesc, day);strcat(fullSTimeDesc, " ");strcat(fullSTimeDesc, month);strcat(fullSTimeDesc, " ");strcat(fullSTimeDesc, year);strcat(fullSTimeDesc, " ");strcat(fullSTimeDesc, sTime);strcat(fullSTimeDesc, ":00");

                strptime(fullSTimeDesc, "%d %m %Y %H:%M:%S", &tmpTm);

                sTime_t = mktime(&tmpTm);

                #ifdef LOGGING_SYSMODES
                    Log.info("[SysModesMngr::restoreScheduler] - Scheduled mode START time: " + String(ctime(&sTime_t)));
                    Log.info("[SysModesMngr::restoreScheduler] - Scheduled mode CURRENT time: " + String(ctime(&currTime_t)));
                #endif

                diff = (difftime(currTime_t, sTime_t)/3600);
                #ifdef LOGGING_SYSMODES
                    Log.info("[SysModesMngr::restoreScheduler] - Calculated different time in hours (current - start time): " + String(diff));
                #endif
            }

            delete [] fullSTimeDesc;

            if (diff < 0 && diff < period/2)
            {   
                tmpPeriod = (period/2) + diff;
            }
            else if (diff > 0 && diff < period/2)
            {
                tmpPeriod = (period/2) - diff;
            }
            #ifdef LOGGING_SYSMODES
                Log.info("[SysModesMngr::restoreScheduler] - Calculated tmp Period: " + String(tmpPeriod));
            #endif
        }
        
        switch(tmpWahoami)
        {
            case SysMode::HIGH_M:
            {
                this->sched.schedPool[i] = new HighSchedMode(SysMode::HIGH_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
            case SysMode::LOW_M:
            {
                this->sched.schedPool[i] = new LowSchedMode(SysMode::LOW_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
            case SysMode::SILENT_M:
            {
                this->sched.schedPool[i] = new SilentSchedMode(SysMode::SILENT_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
            case SysMode::NIGHT_M:
            {
                this->sched.schedPool[i] = new NightSchedMode(SysMode::NIGHT_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
             case SysMode::OFF_M:
            {
                this->sched.schedPool[i] = new OffSchedMode(SysMode::OFF_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
             case SysMode::MANUAL_1_M:
            {
                this->sched.schedPool[i] = new Manual1SchedMode(SysMode::MANUAL_1_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
             case SysMode::MANUAL_2_M:
            {
                this->sched.schedPool[i] = new Manual2SchedMode(SysMode::MANUAL_2_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
             case SysMode::MANUAL_3_M:
            {
                this->sched.schedPool[i] = new Manual3SchedMode(SysMode::MANUAL_3_M, (char*)sTime, period, tmpPeriod, this);
                break;
            }
        }

        if (tmpPeriod > 0.0)
        {
            unsigned int tmpP = int(this->sched.schedPool[i]->tmpPeriod * 60 * 60 * 1000);                              /* Calculate the tmp period in millis */
            #ifdef LOGGING_SYSMODES
                Log.info("[SysModesMngr::restoreScheduler] - Calculated timer tmp period (ms): " + String(tmpP));
            #endif
            this->sched.schedPool[i]->timer = new Timer(tmpP, &Scheduler::schedModeTimerCallback, this->sched, true);   /* Define a tmp timer */
        }
        else
        {
            unsigned int p = int(this->sched.schedPool[i]->period * 0.5 * 60 * 60 * 1000);                              /* Calculdate the period in millis */
            #ifdef LOGGING_SYSMODES
                Log.info("[SysModesMngr::restoreScheduler] - Calculated static timer period (ms): " + String(p));
            #endif
            this->sched.schedPool[i]->timer = new Timer(p, &Scheduler::schedModeTimerCallback, this->sched, true);      /* Define a timer */
        }
    }

    #ifdef LOGGING_SYSMODES
        Log.info("[SysModesMngr::restoreScheduler] - Scheduler restored successfully");
    #endif

    this->sched.startScheduler();
}


SysModeCommStatus SysModesMngr::defScheduler(SysMode *modesArr, char stimes[NUM_OF_SCHEDULERS][6], float *periodsArr, float *tmpPeriodsArr, uint8_t numOfModes)
{
    if (this->sched.size > SchedSize::EMPTY)
        this->delScheduler();

    for (uint8_t i=0; i<numOfModes; i++)
    {
        if (periodsArr[i] <= 0.0 || periodsArr [i] > 24.0)
        {
            for (uint8_t j=i-1; i>=0; j--)
            {
                this->sched.schedPool[j] = new NightSchedMode();
                EepromMngr::set("scheduler", "sched" + String(j + 1), "whoami", (int8_t)this->sched.schedPool[j]->whoami);
                EepromMngr::set("scheduler", "sched" + String(j + 1), "start-time", this->sched.schedPool[j]->sTime);
                EepromMngr::set("scheduler", "sched" + String(j + 1), "period", this->sched.schedPool[j]->period);
                if (j+1 == 1)
                    EepromMngr::set("scheduler", "sched" + String(j + 1), "tmp-period", this->sched.schedPool[j]->tmpPeriod);  /* Update the tmp-period only for the sched1 */
            }

            return SysModeCommStatus::WRONG_PERIOD;
        }

        switch(modesArr[i])
        {
            case SysMode::HIGH_M:
            {
                this->sched.schedPool[i] = new HighSchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
            case SysMode::LOW_M:
            {
                this->sched.schedPool[i] = new LowSchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
            case SysMode::SILENT_M:
            {
                this->sched.schedPool[i] = new SilentSchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
            case SysMode::NIGHT_M:
            {
                this->sched.schedPool[i] = new NightSchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
            case SysMode::OFF_M:
            {
                this->sched.schedPool[i] = new OffSchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
            case SysMode::MANUAL_1_M:
            {
                this->sched.schedPool[i] = new Manual1SchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
            case SysMode::MANUAL_2_M:
            {
                this->sched.schedPool[i] = new Manual2SchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
            case SysMode::MANUAL_3_M:
            {
                this->sched.schedPool[i] = new Manual3SchedMode(modesArr[i], stimes[i], periodsArr[i], tmpPeriodsArr[i], this);
                break;
            }
        }

        if (i == 0 && tmpPeriodsArr[i] > 0.0)
        {
            unsigned int tmpP = int(this->sched.schedPool[i]->tmpPeriod * 60 * 60 * 1000);                                    /* Calculate the tmp period in millis */
            this->sched.schedPool[i]->timer = new Timer(tmpP, &Scheduler::schedModeTimerCallback, this->sched, true);         /* Define a tmp timer */
        }
        else
        {
            unsigned int p = int(this->sched.schedPool[i]->period * 0.5 * 60 * 60 * 1000);                                   /* Calculdate the period in millis */
            this->sched.schedPool[i]->timer = new Timer(p, &Scheduler::schedModeTimerCallback, this->sched, true);           /* Define a timer */
        }

        EepromMngr::set("scheduler", "sched" + String(i + 1), "whoami", (int8_t)this->sched.schedPool[i]->whoami);
        EepromMngr::set("scheduler", "sched" + String(i + 1), "start-time", this->sched.schedPool[i]->sTime);
        EepromMngr::set("scheduler", "sched" + String(i + 1), "period", this->sched.schedPool[i]->period);
        if (i+1 == 1)
            EepromMngr::set("scheduler", "sched" + String(i + 1), "tmp-period", this->sched.schedPool[i]->tmpPeriod);
    }

    this->sched.size = (SchedSize)numOfModes;
    EepromMngr::set("scheduler", "size", NULL, (uint8_t)this->sched.size);

    this->sched.startScheduler();

    // Procedure to copy the scheudler data and publish it 
    // char schedulerData[600];
    // JSONBufferWriter jwriter(schedulerData, sizeof(schedulerData)-1);
    // jwriter.beginObject();
    //     jwriter.name("scheduler").beginObject()
    //         .name("size").value(this->sched.size)
    //         .name("position").value(this->sched.pos);
    //         for (uint8_t i=0; i<this->sched.size; i++)
    //         {
    //             char schedNum[6] = "sched";
    //             strcat(schedNum, String(i + 1));
    //             jwriter.name(schedNum).beginObject()
    //                 .name("whoami").value(this->sched.schedPool[i]->whoami)
    //                 .name("start-time").value(this->sched.schedPool[i]->sTime)
    //                 .name("period").value(this->sched.schedPool[i]->period);
    //                 if (i+1 == 1)
    //                     jwriter.name("tmp-period").value(this->sched.schedPool[i]->tmpPeriod);
    //             jwriter.endObject();
    //         }
    //     jwriter.endObject();
    // jwriter.endObject();
    // Publisher::publishEvent('S', "1.1.10", "support.modes", schedulerData, true);

    return SysModeCommStatus::COMM_SUCCESS;
}


SysModeCommStatus SysModesMngr::delScheduler()
{
    if (this->sched.size == SchedSize::EMPTY)
        return SysModeCommStatus::NO_SCHED_TO_DELETE;

    SchedSize lastSize = this->sched.stopScheduler();
    for (uint8_t i=0; i<lastSize; i++)
    {
        EepromMngr::set("scheduler", "sched" + String(i + 1), "whoami", (int8_t)this->sched.schedPool[i]->whoami);
        EepromMngr::set("scheduler", "sched" + String(i + 1), "start-time", (int8_t)NULL);
        EepromMngr::set("scheduler", "sched" + String(i + 1), "period", this->sched.schedPool[i]->period);
        if (i+1 == 1)
            EepromMngr::set("scheduler", "sched" + String(i + 1), "tmp-period", this->sched.schedPool[i]->tmpPeriod);
    }

    EepromMngr::set("scheduler", "size", NULL, (uint8_t)this->sched.size);
    this->sched.schedModeEnds = false;      /* We need to do this because after timer->stop operation, the callback called and make this variable as True */

    Publisher::publishEvent('S', "1.1.11", "support.modes", "Scheduler definition removed");

    return SysModeCommStatus::COMM_SUCCESS;
}


SysModeCommStatus SysModesMngr::setDefaultMode(SysMode mode)
{
    if (mode == SysMode::NIGHT_M) return SysModeCommStatus::WRONG_DEFAULT_MODE;

    this->defaultMode = mode;
    EepromMngr::set("active-mode", "default", NULL, (int8_t)this->defaultMode);    

    if (!this->sched.schedPool[this->sched.pos]->activeNow/* && (this->quicky == nullptr || !this->quicky->activeNow)*/)
    {
        switch(mode)
        {
            case SysMode::HIGH_M:
            {
                // this->execComponentsChainCallback(SysEvent::FAN_AUTO);
                this->execComponentsChainCallback(SysEvent::ENTER_HIGH_M);
                Publisher::publishEvent('S', "1.1.4", "support.modes", "Enter high mode");
                break;
            }
            case SysMode::LOW_M:
            {
                // this->execComponentsChainCallback(SysEvent::FAN_AUTO);
                this->execComponentsChainCallback(SysEvent::ENTER_LOW_M);
                Publisher::publishEvent('S', "1.1.6", "support.modes", "Enter low mode");
                break;
            }
            case SysMode::SILENT_M:
            {
                // this->execComponentsChainCallback(SysEvent::FAN_AUTO);
                this->execComponentsChainCallback(SysEvent::ENTER_SILENT_M);
                Publisher::publishEvent('S', "1.1.12", "support.modes", "Enter silent mode");
                break;
            }
            case SysMode::OFF_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_OFF_M);
                Publisher::publishEvent('S', "1.1.16", "support.modes", "Enter off mode");
                break;
            }
            case SysMode::MANUAL_1_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_MANUAL_1_M);
                Publisher::publishEvent('S', "1.1.18", "support.modes", "Enter manual 1 mode");
                break;
            }
            case SysMode::MANUAL_2_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_MANUAL_2_M);
                Publisher::publishEvent('S', "1.1.20", "support.modes", "Enter manual 2 mode");
                break;
            }
            case SysMode::MANUAL_3_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_MANUAL_3_M);
                Publisher::publishEvent('S', "1.1.22", "support.modes", "Enter manual 3 mode");
                break;
            }
            default:
                break;
        }

        EepromMngr::set("active-mode", "current", NULL, (int8_t)mode);
    }

    Publisher::publishEvent('S', "1.1.2", "support.modes", "Default mode was set to: " + String(mode));

    return SysModeCommStatus::COMM_SUCCESS;
}


SysModeCommStatus SysModesMngr::resetDefaultMode(SysMode mode)
{
    int8_t tmpDefMode;
    EepromMngr::get("active-mode", "default", NULL, &tmpDefMode);
    if ((SysMode)tmpDefMode != mode) return SysModeCommStatus::CANT_TURN_OFF_MODE_THAT_IS_NOT_ON;

    this->defaultMode = SysMode::AUTO_M;
    EepromMngr::set("active-mode", "default", NULL, (int8_t)this->defaultMode);
    this->activateDefaultMode(this->defaultMode);
    Publisher::publishEvent('S', "1.1.3", "support.modes", "Default mode restarted to Auto");
    return SysModeCommStatus::COMM_SUCCESS;
}


void SysModesMngr::activateDefaultMode(SysMode mode)
{    
    if (mode == SysMode::NOT_INIT)
    {
        int8_t tmpDefMode;
        EepromMngr::get("active-mode", "default", NULL, &tmpDefMode);
        this->defaultMode = (SysMode)tmpDefMode;
    }

    if (!this->sched.schedPool[this->sched.pos]->activeNow/* && (this->quicky == nullptr || !this->quicky->activeNow)*/)
    {
        switch(this->defaultMode)
        {
            case SysMode::AUTO_M:
            {
                //get the auto silent switch state from EEPROM
                bool autoSilentSwitch;
                EepromMngr::get("active-mode", "auto-silent-switch", NULL, &autoSilentSwitch);
                if(autoSilentSwitch){
                    this->execComponentsChainCallback(SysEvent::AUTO_SILENT_ON);
                    Publisher::publishEvent('S', "1.1.18", "support.modes", "Set auto silent mode on");
                }else{
                    this->execComponentsChainCallback(SysEvent::AUTO_SILENT_OFF);
                    Publisher::publishEvent('S', "1.1.19", "support.modes", "Set auto silent mode off");
                }

                // this->execComponentsChainCallback(SysEvent::FAN_AUTO);
                this->execComponentsChainCallback(SysEvent::ENTER_AUTO_M);
                Publisher::publishEvent('S', "1.1.0", "support.modes", "Enter auto mode");
                break;
            }
            case SysMode::HIGH_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_HIGH_M);
                Publisher::publishEvent('S', "1.1.4", "support.modes", "Enter high mode");
                break;
            }
            case SysMode::LOW_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_LOW_M);
                Publisher::publishEvent('S', "1.1.6", "support.modes", "Enter low mode");
                break;
            }
            case SysMode::SILENT_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_SILENT_M);
                Publisher::publishEvent('S', "1.1.12", "support.modes", "Enter silent mode");
                break;
            }
            case SysMode::OFF_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_OFF_M);
                Publisher::publishEvent('S', "1.1.16", "support.modes", "Enter off mode");
                break;
            }
            case SysMode::MANUAL_1_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_MANUAL_1_M);
                Publisher::publishEvent('S', "1.1.18", "support.modes", "Enter manual 1 mode");
                break;
            }
            case SysMode::MANUAL_2_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_MANUAL_2_M);
                Publisher::publishEvent('S', "1.1.20", "support.modes", "Enter manual 2 mode");
                break;
            }
            case SysMode::MANUAL_3_M:
            {
                this->execComponentsChainCallback(SysEvent::ENTER_MANUAL_3_M);
                Publisher::publishEvent('S', "1.1.22", "support.modes", "Enter manual 3 mode");
                break;
            }
            default:
                break;
        }

        EepromMngr::set("active-mode", "current", NULL, (int8_t)this->defaultMode);
    }
}


/*SysModeCommStatus SysModesMngr::defQuickMode()
{
    if (this->quicky != nullptr) return SysModeCommStatus::QUICK_MODE_ALREADY_ACTIVATED;

    this->quicky = new QuickyMode(this);
    this->quicky->timer = new Timer(this->quicky->period * 60 * 60 * 1000, &SysModesMngr::delQuickMode, *this, true);
    this->quicky->enterMode();
    
    return SysModeCommStatus::COMM_SUCCESS;
}


void SysModesMngr::delQuickMode()
{
    if (this->quicky != nullptr)
        this->quickModeEnds = true;
}*/


void SysModesMngr::inspectScheduledModesState()
{
    /*if (this->quickModeEnds)
    {
        this->quickModeEnds = false;
        this->quicky->exitMode();
        delete this->quicky;
        this->quicky = nullptr;
        this->activateDefaultMode();
        this->restoreScheduler();
    }*/

    if (this->sched.size > SchedSize::EMPTY)
    {
        if (!this->sched.schedPool[this->sched.pos]->activeNow)
        {
            #ifdef LOGGING_SYSMODES
                Log.trace("[SysModesMngr::inspectScheduledModesState] - Trying to find entry point for: " + String(this->sched.schedPool[this->sched.pos]->whoami));
            #endif
            this->sched.findSchedEntryPoint();
        }
        else if (this->sched.schedPool[this->sched.pos]->activeNow && this->sched.schedModeEnds)
        {
            #ifdef LOGGING_SYSMODES
                Log.info("[SysModesMngr::inspectScheduledModesState] - The scheduled mode: " + String(this->sched.schedPool[this->sched.pos]->whoami) + " ended. Switching positions");
            #endif
            this->sched.switchPosition();
        }
    }
}