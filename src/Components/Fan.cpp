#include "Fan.h"

Fan *Fan::instance = nullptr;
int Fan::rpmCounter = 0;
volatile uint32_t Fan::rpmPulseCount = 0;
uint32_t          Fan::rpmWindowStart = 0;
uint16_t          Fan::lastCalculatedRpm = 0;
bool              Fan::tachoAttached = false;

Fan::Fan() : pid(PIDImpl(500, -500, 0.5, 0.0, 0.0))
{
    this->fanSpeed = (double)FAN_OFF;
    this->tmpFanSpeed = (double)FAN_OFF;
    this->pwm = MAX_PWM;

    this->constSwitch = SysEvent::FAN_AUTO;

    this->fanArrCurrIdx = 0;
    for (uint8_t i = 0; i < FAN_SPEEDS_ARR_SIZE; i++)
        this->fanSpeedArr[i] = (uint16_t)FAN_LVL_0;

    this->drivePid = true;
    this->lastPidCompute = 0;

    this->lastFanInspection = 0;

    this->inSysMode = false;
    this->coverOpen = false;
    this->isAutoSilent = false;
}

Fan &Fan::getInstance()
{
    if (instance == nullptr)
        instance = new Fan();

    return *instance;
}

void Fan::setup()
{
    pinMode(FAN_PIN, OUTPUT);
    pinMode(FAN_TACHO, INPUT);

    analogWriteResolution(FAN_PIN, 8);
    analogWrite(FAN_PIN, 255, 1000);

    // Attach tacho ISR once — runs forever, non-blocking
    attachInterrupt(FAN_TACHO, rpmCounterIsr, RISING);
    rpmWindowStart = millis();
}


void Fan::runPid()
{
    if (this->drivePid)
        this->_drivePid();
}


/**
 * This function restored the LED switch state from EEPROM 
 */
void Fan::restoreSwitchState()
{
    #ifdef LOGGING_LED
        Log.trace("[Fan::restoreSwitchState] - Restore the Fan switch state...");
    #endif
    bool switchState;
    EepromMngr::get("fan-switch", "state", &switchState);
    if (!switchState)
    {
        #ifdef LOGGING_LED
            Log.trace("[Fan::restoreSwitchState] - Turning off the Fan");
        #endif
        this->constSwitch = SysEvent::FAN_MANU_BY_PID;
        this->handleEvent(SysEvent::FAN_MANU_BY_PID);
        return;
    }

    #ifdef LOGGING_LED
        Log.trace("[Fan::restoreSwitchState] - Nothing to apply");
    #endif
}

// non-blocking
uint16_t Fan::getFanRpm()
{
    uint32_t now = millis();
    uint32_t elapsed = now - rpmWindowStart;

    if (elapsed >= 1000)   // compute every 1 second window
    {
        uint32_t pulses = rpmPulseCount;   // snapshot
        rpmPulseCount = 0;                 // reset counter
        rpmWindowStart = now;
        lastCalculatedRpm = (pulses * 60) / 2;
    }

    return lastCalculatedRpm;
}

void Fan::rpmCounterIsr()
{
    rpmPulseCount++;
}

void Fan::_drivePid()
{
    if (millis() - this->lastPidCompute < PID_COMPUTATION_INTERVAL)
        return;

    uint8_t tmpPwm = this->pwm;

    uint16_t rpm = this->getFanRpm();
#ifdef LOGGING_FAN
    Log.info("[Fan::_drivePid] - Current RPM: %d", rpm);
#endif
    double inc = pid.calculate(this->fanSpeed, rpm);

    if (inc >= 500)
        tmpPwm -= 16;
    else if (inc >= 400)
        tmpPwm -= 8;
    else if (inc >= 300)
        tmpPwm -= 4;
    else if (inc >= 60)
        tmpPwm -= 2;
    else if (inc >= 45)
        tmpPwm -= 1;
    else if (inc <= -45)
        tmpPwm += 1;
    else if (inc <= -60)
        tmpPwm += 2;
    else if (inc <= -300)
        tmpPwm += 4;
    else if (inc <= -400)
        tmpPwm += 8;
    else if (inc <= -500)
        tmpPwm += 16;

    if (tmpPwm > MAX_PWM)
        tmpPwm = MAX_PWM;
    else if (tmpPwm < MIN_PWM)
        tmpPwm = MIN_PWM;

    if (this->pwm != tmpPwm)
    {
        analogWrite(FAN_PIN, tmpPwm);
        this->pwm = tmpPwm;
    }

    this->lastPidCompute = millis();
}

/**
 * This function received a speed and store it inside an array.
 * Once all the array filled with the same speed, this function set the fan speed of the fan to the requested speed.
 *
 * @param speed to set the fan to
 */
void Fan::setFanSpeed(double speed, bool constFanSpeedFlag)
{
    if (constFanSpeedFlag)
    {
        if (speed == (double)FAN_OFF)
        {
            analogWrite(FAN_PIN, 255, 1000);
            this->drivePid = false;
#ifdef LOGGING_FAN
            Log.trace("[Fan::setFanSpeed] - Fan turned off");
#endif
        }
        else
        {
            this->fanSpeed = speed;
#ifdef LOGGING_FAN
            Log.trace("[Fan::setFanSpeed] - Set the fan speed to: %d", (uint16_t)this->fanSpeed);
#endif
        }

        return;
    }

    if (this->fanArrCurrIdx == FAN_SPEEDS_ARR_SIZE)
        this->fanArrCurrIdx = 0;

    this->fanSpeedArr[this->fanArrCurrIdx++] = (uint16_t)speed;

    if (!this->inSysMode && !coverOpen)
    {
        // Check if all the array filled with the same fan speed
        for (uint8_t i = 0; i < FAN_SPEEDS_ARR_SIZE; i++)
        {
            if (this->fanSpeedArr[i] != (uint16_t)speed)
                return;
        }

        this->fanSpeed = speed;
#ifdef LOGGING_FAN
        Log.trace("[Fan::setFanSpeed] - Set the fan speed to: %d", (uint16_t)this->fanSpeed);
#endif
    }
}

void Fan::setFanSpeed(uint8_t pwmSpeed)
{
    if (this->coverOpen)
        return;
    if (pwmSpeed == (double)FAN_OFF)
    {
        analogWrite(FAN_PIN, 255, 1000);
        this->drivePid = false;
#ifdef LOGGING_FAN
        Log.trace("[Fan::setFanSpeed] - Fan turned off");
#endif
    }
    else
    {
         if(this->drivePid == false)
        {
            this->drivePid = true;
        }
#ifdef LOGGING_FAN
        Log.trace("[Fan::setFanSpeed] - Set the fan speed to: %d", pwmSpeed);
#endif
        analogWrite(FAN_PIN, pwmSpeed, 1000);
    }
}

void Fan::setFanSpeed(uint16_t rpmSpeed)
{
    if (this->coverOpen)
        return;

#ifdef LOGGING_FAN
    Log.trace("[Fan::setFanSpeed] - Set the fan speed to: %d", rpmSpeed);
#endif
    this->tmpFanSpeed = this->fanSpeed;
    this->fanSpeed = rpmSpeed;
}

void Fan::handleEvent(SysEvent e)
{
    switch (e)
    {
    case SysEvent::AUTO_SILENT_ON:
    {
        this->isAutoSilent = true;
        break;
    }
    case SysEvent::AUTO_SILENT_OFF:
    {
        this->isAutoSilent = false;
        break;
    }
    case SysEvent::COVER_OPEN:
    {
        this->coverOpen = true;
        this->setFanSpeed(FAN_OFF, true);
        break;
    }
    case SysEvent::COVER_CLOSE:
    {
        this->coverOpen = false;
        this->drivePid = true;
        break;
    }
    case SysEvent::AQI_LESS_50:
    {
        if(this->isAutoSilent)
        {
            if (this->fanSpeed != (double)FAN_AS_LVL_0)
                this->setFanSpeed((double)FAN_AS_LVL_0);
        }
        else
        {
            if (this->fanSpeed != (double)FAN_LVL_0)
                this->setFanSpeed((double)FAN_LVL_0);
        }
        break;
    }
    case SysEvent::AQI_LESS_100:
    {
        if(this->isAutoSilent)
        {
            if (this->fanSpeed != (double)FAN_AS_LVL_1)
                this->setFanSpeed((double)FAN_AS_LVL_1);
        }
        else
        {
            if (this->fanSpeed != (double)FAN_LVL_1)
                this->setFanSpeed((double)FAN_LVL_1);
        }
        break;
    }
    case SysEvent::AQI_LESS_200:
    {
        if(this->isAutoSilent)
        {
            if (this->fanSpeed != (double)FAN_AS_LVL_2)
                this->setFanSpeed((double)FAN_AS_LVL_2);
        }
        else
        {
            if (this->fanSpeed != (double)FAN_LVL_2)
                this->setFanSpeed((double)FAN_LVL_2);
        }
        break;
    }
    case SysEvent::AQI_LESS_500:
    {
        if(this->isAutoSilent)
        {
            if (this->fanSpeed != (double)FAN_AS_LVL_3)
                this->setFanSpeed((double)FAN_AS_LVL_3);
        }
        else
        {
            if (this->fanSpeed != (double)FAN_LVL_3)
                this->setFanSpeed((double)FAN_LVL_3);
        }
        break;
    }
    case SysEvent::ENTER_AUTO_M:
    case SysEvent::ENTER_LISTENING_M:
    {
        this->inSysMode = false;
         
         if (!this->coverOpen)
            this->drivePid = true;

        if (this->tmpFanSpeed != (double)FAN_OFF)
        {
            this->fanSpeed = this->tmpFanSpeed;
            this->tmpFanSpeed = (double)FAN_OFF;
        }

        Publisher::publishEvent('S', "1.4.2", "support.fan", "Fan on (ENTER_AUTO_MODE)");
        break;
    }
    case SysEvent::ENTER_HIGH_M:
    case SysEvent::ENTER_LOW_M:
    /*case SysEvent::ENTER_QUICK_M:*/
    case SysEvent::ENTER_SILENT_M:
    case SysEvent::ENTER_NIGHT_M:
    case SysEvent::ENTER_MANUAL_1_M:
    case SysEvent::ENTER_MANUAL_2_M:
    case SysEvent::ENTER_MANUAL_3_M:
    {
        this->inSysMode = true;
        this->drivePid = true;
        Publisher::publishEvent('S', "1.4.2", "support.fan", "Fan on (ENTER_HIGH_M/LOW/SILENT/NIGHT)");

        if (e == ENTER_HIGH_M)
            this->setFanSpeed(HIGH_FAN_LVL, true);
        else if (e == SysEvent::ENTER_LOW_M || e == SysEvent::ENTER_QUICK_M)
            this->setFanSpeed(LOW_FAN_LVL, true);
        else if (e == SysEvent::ENTER_SILENT_M)
            this->setFanSpeed(SILENT_FAN_LVL, true);
        else if (e == SysEvent::ENTER_NIGHT_M)
            this->setFanSpeed(NIGHT_FAN_LVL, true);
        else if (e == SysEvent::ENTER_MANUAL_1_M)
            this->setFanSpeed(FAN_MANUAL_1, true);
        else if (e == SysEvent::ENTER_MANUAL_2_M)
            this->setFanSpeed(FAN_MANUAL_2, true);
        else if (e == SysEvent::ENTER_MANUAL_3_M)
            this->setFanSpeed(FAN_MANUAL_3, true);
        break;
    }
    case SysEvent::ENTER_OFF_M:
    {
        Publisher::publishEvent('S', "1.4.3", "support.fan", "Fan off (ENTER_OFF_M)");
        this->inSysMode = true;
        this->setFanSpeed(FAN_OFF, true);
        // TODO - Implement the off mode
        break;
    }
    case SysEvent::FAN_AUTO:
    {
        Publisher::publishEvent('S', "1.4.2", "support.fan", "Fan on (FAN_AUTO)");
        if (!this->coverOpen)
            this->drivePid = true;

        if (this->tmpFanSpeed != (double)FAN_OFF)
        {
            this->fanSpeed = this->tmpFanSpeed;
            this->tmpFanSpeed = (double)FAN_OFF;
        }
        
        break;
    }
    case SysEvent::FAN_MANU_BY_PWM:
    {
        this->drivePid = false;
        break;
    }
    case SysEvent::FAN_MANU_BY_PID:
    {
        if (!this->coverOpen)
            this->drivePid = true;
        break;
    }
    default:
        break;
    }

    if (this->drivePid)
        this->_drivePid();

    if (millis() - this->lastFanInspection > FAN_INSPEC_INTERVAL)
    {
        if (!this->coverOpen && this->drivePid && this->getFanRpm() == 0)
            Publisher::publishEvent('S', "1.4.1", "support.fan", "Fan doesn't work properly");

        this->lastFanInspection = millis();
    }

    Components::handleEvent(e);
}