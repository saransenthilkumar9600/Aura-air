#include "SysMngr.h"


SysMngr::SysMngr() : led(Led::getInstance()), fan(Fan::getInstance()), uvc(Uvc::getInstance()), sterio(Sterionizer::getInstance()),
                     coverMngr(CoverMngr::getInstance()), aqiLyzer(AqiAnalyzer::getInstance()), modesMngr(SysModesMngr::getInstance()),
                     recovMngr(RecoverMngr::getInstance()), connMngr(ConnectivityMngr::getInstance())
{
    this->led.setNextHandler(&this->fan);
    this->led.addNextHandler(&this->sterio);
    this->led.addNextHandler(&this->uvc);

    this->execComponentsChainCallback = std::bind(&SysMngr::execComponentsChain, this, _1);
}


void SysMngr::setup()
{
    #ifdef LOGGING_SYSMNGR
        Log.trace("[SysMngr::setup] - Setup start");
    #endif

    Wire.begin();

    EepromMngr::initEeprom();

    this->coverMngr.lateSetup(this->execComponentsChainCallback);
    this->aqiLyzer.setup(this->execComponentsChainCallback);
    this->modesMngr.setup(this->execComponentsChainCallback);

    this->led.restoreSwitchState();
    this->led.applySchedulerCheck();
    this->sterio.restoreSwitchState();
    this->uvc.restoreSwitchState();
    // this->fan.restoreSwitchState();

    Particle.variable("coverState", coverMngr.getCoverState());
    this->setTimeZone("resotreTimeZone");

    //publishqueue
    Publisher::setup();

    // If the reset occurred because of 'Power Down' event, we want to postponed the scheduler restore 
    // after Internet connection established. The restore of a scheduler is based on the internal clock which should
    // be syncronized (with Particle Console) at this moment
    this->recovMngr.recover();
    if (!this->recovMngr.getPowerDownResetOccurred())
        this->modesMngr.restoreScheduler();

    #ifdef LOGGING_SYSMNGR
        Log.trace("[SysMngr::setup] - Setup done");
    #endif
}


void SysMngr::run()
{
    this->connMngr.run();

    this->modesMngr.inspectScheduledModesState();

    this->coverMngr.inspectCoverState();

    this->led.applySchedulerCheck();

    this->aqiLyzer.run();
}


void SysMngr::execComponentsChain(SysEvent e)
{
    #ifdef LOGGING_SYSMNGR
        Log.trace("[SysMngr::execComponentsChain] - Event %d received", (uint8_t)e);
    #endif
    this->led.handleEvent(e);
}


bool SysMngr::parseSingleSchedMode(String schedModeBlock, char* sTime, float *period, float *tmpPeriod)
{
    uint8_t undScrCount = 0, undScrIdx = 0;
    char spliter = '_';

    for (uint8_t i = 0; i < schedModeBlock.length(); i++)
    {
        if (schedModeBlock.charAt(i) == spliter)
        {
            undScrCount++;
            switch(undScrCount)
            {
                case 1:
                {
                    if (schedModeBlock.charAt(i+1) == '0') 
                        return false;
                    break;
                }
                case 2:
                {
                    undScrIdx=i;
                    break;
                }
                case 3: 
                {
                    /* Extract the start time (HH:MM) and save the index of the 2nd underscore */
                    schedModeBlock.substring(undScrIdx + 1, i).toCharArray(sTime, 6);
                    undScrIdx = i;
                    break;
                }
                case 4: 
                {
                    /* If we find the 4th underscore, extract both the period and tmp period */
                    char tmpTimerLen[20];
                    schedModeBlock.substring(undScrIdx+1, i).toCharArray(tmpTimerLen, 20);
                    *period = String(tmpTimerLen).toFloat();

                    /* Extract the tmp period */
                    char tmpTimerLen2[20];
                    schedModeBlock.substring(i + 1, schedModeBlock.length()).toCharArray(tmpTimerLen2, 20);
                    *tmpPeriod = String(tmpTimerLen2).toFloat();
                    break;
                }
            }
        }
    }

    /* If after the parsing there is only 3 underscores, its mean that we never find the 4th underscore and there is no tmp period in the input */
    if (undScrCount == 3)
    {
        char tmpTimerLen[20];
        schedModeBlock.substring(undScrIdx + 1, schedModeBlock.length()).toCharArray(tmpTimerLen, 20);
        *period = String(tmpTimerLen).toFloat();
        *tmpPeriod = 0.0;
    }
    
    return true;
}


/**
 * This is a Particle cloud function
 * 
 * @param data should leave empty
 * @return 1 for success
 */
int SysMngr::printEeprom(String data)
{
    this->connMngr.setEepromContent();
    return 1;
}


int SysMngr::getModeFromString(String data)
{
    uint8_t mode;

    String firstTwoLetters = data.substring(0, 2);
    //support 2 letter digits as well.
    if (firstTwoLetters == "1_") mode = SysMode::HIGH_M;
    else if (firstTwoLetters == "2_") mode = SysMode::LOW_M;
    else if (firstTwoLetters == "3_") mode = SysMode::NIGHT_M;
    else if (firstTwoLetters == "4_") mode = SysMode::SCHEDULER_M;
    else if (firstTwoLetters == "5_") mode = SysMode::SILENT_M;
    else if (firstTwoLetters == "6_") mode = SysMode::MANUAL_1_M;
    else if (firstTwoLetters == "7_") mode = SysMode::OFF_M;
    else if (firstTwoLetters == "8_") mode = SysMode::MANUAL_2_M;
    else if (firstTwoLetters == "9_") mode = SysMode::MANUAL_3_M;
    else if (firstTwoLetters == "10") mode = SysMode::AUTO_SILENT;

    return mode;
}

/**
 * This is a Particle cloud function.
 * 
 * This function parses the received input parameter according 
 * to agreed protocol and then apply the request on the system
 * 
 * @param data string representation of a request
 * @return 0 for failure, 1 for success, otherwise according to the enum 'SysModeCommStatus'
 */
int SysMngr::setSystemMode(String data)
{
    Publisher::publishEvent('S', "1.1.1", "support.modes", data);
    //support 2 letter digits.
    uint8_t mode = getModeFromString(data);
    
    if (mode == SysMode::AUTO_SILENT)
    {
        data.remove(0, 3);
        if (data == "on") {
            this->execComponentsChain(SysEvent::AUTO_SILENT_ON);
            EepromMngr::set("active-mode", "auto-silent-switch", NULL, true);
            Publisher::publishEvent('S', "1.1.18", "support.modes", "Set auto silent mode on");
        }
        else if (data == "off"){
            this->execComponentsChain(SysEvent::AUTO_SILENT_OFF);
            EepromMngr::set("active-mode", "auto-silent-switch", NULL, false);
            Publisher::publishEvent('S', "1.1.19", "support.modes", "Set auto silent mode off");
        } 
        else return SysModeCommStatus::WRONG_AUTO_SILENT_INPUT;
    }
    if (mode == SysMode::HIGH_M)
    {
        data.remove(0, 2);
        if (data == "on") return (int)this->modesMngr.setDefaultMode(SysMode::HIGH_M);
        else if (data == "off") return (int)this->modesMngr.resetDefaultMode(SysMode::HIGH_M);
        else return SysModeCommStatus::WRONG_HIGH_MODE_INPUT;
    }
    else if (mode == SysMode::LOW_M)
    {
        data.remove(0, 2);
        if (data == "on") return (int)this->modesMngr.setDefaultMode(SysMode::LOW_M);
        else if (data == "off") return (int)this->modesMngr.resetDefaultMode(SysMode::LOW_M);
        else return SysModeCommStatus::WRONG_LOW_MODE_INPUT;
    }
    else if (mode == SysMode::SILENT_M)
    {
        data.remove(0, 2);
        if (data == "on") return (int)this->modesMngr.setDefaultMode(SysMode::SILENT_M);
        else if (data == "off") return (int)this->modesMngr.resetDefaultMode(SysMode::SILENT_M);
        else return WRONG_SILENT_MODE_INPUT;
    }
    else if (mode == SysMode::OFF_M)
    {
        data.remove(0, 2);
        if (data == "on") return (int)this->modesMngr.setDefaultMode(SysMode::OFF_M);
        else if (data == "off") return (int)this->modesMngr.resetDefaultMode(SysMode::OFF_M);
        else return WRONG_FAN_MODE_INPUT;
    }
    else if (mode == SysMode::MANUAL_1_M)
    {
        data.remove(0, 2);
        if (data == "on") return (int)this->modesMngr.setDefaultMode(SysMode::MANUAL_1_M);
        else if (data == "off") return (int)this->modesMngr.resetDefaultMode(SysMode::MANUAL_1_M);
        else return WRONG_FAN_MODE_INPUT;
    }
        else if (mode == SysMode::MANUAL_2_M)
    {
        data.remove(0, 2);
        if (data == "on") return (int)this->modesMngr.setDefaultMode(SysMode::MANUAL_2_M);
        else if (data == "off") return (int)this->modesMngr.resetDefaultMode(SysMode::MANUAL_2_M);
        else return WRONG_FAN_MODE_INPUT;
    }
        else if (mode == SysMode::MANUAL_3_M)
    {
        data.remove(0, 2);
        if (data == "on") return (int)this->modesMngr.setDefaultMode(SysMode::MANUAL_3_M);
        else if (data == "off") return (int)this->modesMngr.resetDefaultMode(SysMode::MANUAL_3_M);
        else return WRONG_FAN_MODE_INPUT;
    }
    else if (mode == SysMode::NIGHT_M)
    {
        if (data != "3_00:00_0" && data.length() < 9 && data != "3_0") return SysModeCommStatus::WRONG_NIGHT_MODE_INPUT;

        char stimeArr[1][6];
        float periodArr[] = {0.0}, tmpPeroidArr[] = {0.0};
        SysMode modesArr[] = {SysMode::NIGHT_M};

        if (!this->parseSingleSchedMode(data, stimeArr[0], &periodArr[0], &tmpPeroidArr[0]))
            return this->modesMngr.delScheduler();
            
        return this->modesMngr.defScheduler(modesArr, stimeArr, periodArr, tmpPeroidArr, 1);
    }
    else if (mode == SysMode::SCHEDULER_M)
    {
        if (data.length() < 17 && data != "4_0") return SysModeCommStatus::WRONG_SCHEDULER_INPUT;

        uint8_t undScrCount = 0, undScrIdx = 0, tildaIdx = 0, generalIdx = 0;
        const char spliter = '_', tilda = '~';

        char stimeArr[6][6];
        float periodArr[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, tmpPeroidArr[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        SysMode modesArr[] = {SysMode::NOT_INIT, SysMode::NOT_INIT, SysMode::NOT_INIT, SysMode::NOT_INIT, SysMode::NOT_INIT, SysMode::NOT_INIT};

        for (uint8_t i = 0; i < data.length(); i++)
        {
            if (data.charAt(i) == spliter)
            {
                undScrCount++;
                switch (undScrCount)
                {
                    case 1:
                    {
                        if (data.charAt(i + 1) == '0') return this->modesMngr.delScheduler();
                    }
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    {
                        if (data.charAt(i+1) == tilda)
                        {
                            tildaIdx = i+1;
                            i+=2;
                            uint8_t count=0;
                            while(data.charAt(i) != tilda && count < MAX_SCHED_MODE_INPUT_SIZE)  
                            {
                                if (count == 1)
                                    modesArr[generalIdx] = (SysMode)(data.substring(tildaIdx+1, i).toInt());

                                count++;
                                i++;
                            }

                            this->parseSingleSchedMode(data.substring(tildaIdx+1, i), stimeArr[generalIdx], &periodArr[generalIdx], &tmpPeroidArr[generalIdx]);
                            generalIdx++;
                        }
                        break;
                    }
                }
            }
        }

        return this->modesMngr.defScheduler(modesArr, stimeArr, periodArr, tmpPeroidArr, generalIdx);
    }
    else return SysModeCommStatus::WRONG_COMM;

    return SysModeCommStatus::COMM_SUCCESS;
}


/**
 * This cloud function controls the fan manually (mainly for testing purposes).
 * 
 * There are two ways to control the fan: 
 * 1. PWM - value in range [0-255] will drive the fan by PWM 
 *          while the value 256 will turn off the manual fan control.
 *          NOTICE!! that in this case, opening of the front cover will do the same as the value 256
 *          but still you need to push the value 256 so the device will now that you are no longer in 
 *          manual fan control.
 * 
 * 2. PID (RPM) - value in range[300-3300] will drive the fan by the PID (which mean by RPM)
 *                while the value 3301 will turn off the manual fan control
 * 
 * @param data string representation of an integer value
 * @return 0 or 1 for failure or success
 */
int SysMngr::setFanSpeed(String data)
{
    int fanSpeed = data.toInt();
    if (fanSpeed == 0 || fanSpeed == 300){
        // EepromMngr::set("fan-switch", "state", false);
    }
    if (fanSpeed >= 0 && fanSpeed <= 255)           // Control the fan by PWM
    {
        this->execComponentsChain(SysEvent::FAN_MANU_BY_PWM);
        this->fan.setFanSpeed((uint8_t)fanSpeed);
    }
    else if (fanSpeed >= 300 && fanSpeed <= 3300)   // Control the fan by PID
    {
        this->execComponentsChain(SysEvent::FAN_MANU_BY_PID);
        this->fan.setFanSpeed((uint16_t)fanSpeed);
    }
    else if (fanSpeed == 3301 || fanSpeed == 256)   // Cancel the control on the fan by PID
    {
        // EepromMngr::set("fan-switch", "state", true);
        this->execComponentsChain(SysEvent::FAN_AUTO);
    }
    else return 0;

    return 1;
}


int SysMngr::getFanRpm(String data)
{
    return this->fan.getFanRpm();
}


/**
 * This is a Particle cloud function
 * which used to constantly turn on/off the LED
 * 
 * @param data 'on' or 'off'
 * @return 1 for success, 0 for failure
 */
int SysMngr::ledSwitch(String data)
{
    if (data == "on") this->execComponentsChain(SysEvent::LED_ON);
    else if (data == "off") this->execComponentsChain(SysEvent::LED_OFF);
    else return 0;

    return 1;
}


/**
 * Particle cloud function to set the LED scheduler.
 * @param data "HH:mm,HH:mm,on" or "HH:mm,HH:mm,off" (e.g. "09:00,17:00,on"), or "disable" to disable.
 * @return 1 for success, 0 for invalid input
 */
int SysMngr::setLedScheduler(String data)
{
    return this->led.setLedScheduler(data);
}


/**
 * This is a Particle cloud function
 * which used to constantly turn on/off the Sterionizer
 * 
 * @param data 'on' / 'off' / 'auto'
 * @return 1 for success, 0 for failure
 */
int SysMngr::sterioSwitch(String data)
{
    if (data == "on") this->execComponentsChain(SysEvent::STERIO_CONST_ON);
    else if (data == "off") this->execComponentsChain(SysEvent::STERIO_CONST_OFF);
    else if (data == "auto") this->execComponentsChain(SysEvent::STERIO_AUTO);
    else return 0;

    return 1;
}


int SysMngr::getSterioDiagnos(String date)
{
    return this->sterio.getSterioDiagnos();
}


/**
 * This is a Particle cloud function
 * which used to constantly turn on/off the Sterionizer
 * 
 * @param data 'on' / 'off' / 'auto'
 * @return 1 for success, 0 for failure
 */
int SysMngr::uvcSwitch(String data)
{
    if (data == "on") this->execComponentsChain(SysEvent::UVC_CONST_ON);
    else if (data == "off") this->execComponentsChain(SysEvent::UVC_CONST_OFF);
    else if (data == "auto") this->execComponentsChain(SysEvent::UVC_AUTO);
    else return 0;

    return 1;
}


/**
 * This is a Particle cloud function
 * 
 * In case this function gets 'resotreTimeZone' keyword as an argument, 
 * he knows to restore the timezone from EEPROM
 * 
 * @param data String variable represent a timezone. Available inputs: [-12, -11, ..., 13,14, 'resotreTimeZone']
 * @return 0 for failure, 1 for success
 */
int SysMngr::setTimeZone(String data)
{
    if (data == "resotreTimeZone")
    {
        // Restoring timezone data from the EEPROM (if it's stored)
        int8_t timezone;
        EepromMngr::get("timezone", NULL, NULL, &timezone);
        if (timezone != NULL) 
        {
            #ifdef LOGGING_SYSMNGR
                Log.info("[SysMngr::setup] - Restored timezone is: %d", timezone);
            #endif
            Time.zone(timezone);
        }
    }
    else
    {
        int8_t timezone = data.toInt();
        if (timezone > 14 || timezone < -12) 
        {
            Publisher::publishEvent('S', "1.3.1", "support.timezone", "Invalid timezone value");
            return 0;
        }
        Time.zone(timezone);
        EepromMngr::set("timezone", NULL, NULL, (int8_t)timezone);
        Publisher::publishEvent('S', "1.3.2", "support.timezone", "Timezone " + String(timezone) + " was set & saved to EEPROM successfully");
    }
    
    return 1;
}


int SysMngr::setCreds(String data) // Input = 'removeAll'/'SSID~password'
{
    if (data == "removeAll")
    {
        this->execComponentsChain(SysEvent::LED_ON);
        this->connMngr.removeCreds();
        return 1;
    }
    else
    {
        this->connMngr.setNewCreds(data);
        return 1;
    }

    return 0;
}


int SysMngr::printCreds(String data)
{
    this->connMngr.printCreds();
    return 1;
}