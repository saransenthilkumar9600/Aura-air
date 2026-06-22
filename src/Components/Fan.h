#ifndef FAN_H
#define FAN_H


#include "Publisher/Publisher.h"
#include "EepromMngr/EepromMngr.h"
#include "Components.h"
#include "Particle.h"
#include "PIDImpl.h"


// #define LOGGING_FAN
#define HW_VER             1
#if HW_VER == 1
    #define FAN_PIN         P1S0
    #define FAN_TACHO       P1S3
#endif
#define PID_COMPUTATION_INTERVAL    2000
#define FAN_INSPEC_INTERVAL         43200000 //12h
#define FAN_SPEEDS_ARR_SIZE         10
#define MAX_PWM                     50
#define MIN_PWM                     4
#define SILENT_FAN_LVL              500
#define NIGHT_FAN_LVL               900 //28
#define LOW_FAN_LVL                 1200 //22
#define FAN_LVL_0                   1200 //22
#define FAN_LVL_1                   1400 //18
#define FAN_LVL_2                   1800 //12 
#define FAN_LVL_3                   2000 //8
#define FAN_AS_LVL_0                500 //auto silent
#define FAN_AS_LVL_1                900 //auto silent
#define FAN_AS_LVL_2                1200 //auto silent
#define FAN_AS_LVL_3                1400 //auto silent
#define HIGH_FAN_LVL                2000 //8
#define FAN_OFF                     0 //255
#define FAN_MANUAL_1                900 //28
#define FAN_MANUAL_2                1500 //255
#define FAN_MANUAL_3                2500

class Fan : public Components
{
private:
    static Fan *instance;
    double fanSpeed;
    double tmpFanSpeed;
    uint16_t fanSpeedArr[FAN_SPEEDS_ARR_SIZE];
    uint8_t fanArrCurrIdx, pwm;
    bool inSysMode, coverOpen, drivePid, isAutoSilent;
    double currRpm, destRpm;
    unsigned long lastPidCompute;
    unsigned long lastFanInspection;
    static int rpmCounter;
    PIDImpl pid;
    SysEvent constSwitch;

    Fan();
    void setFanSpeed(double, bool constFanSpeedFlag=false);
    static void rpmCounterIsr();
    void _drivePid();

public:
    static Fan& getInstance();
    static void setup();
    void setFanSpeed(uint8_t);
    void setFanSpeed(uint16_t);
    double getFanSpeed() { return this->fanSpeed; }
    uint16_t getFanRpm();
    void restoreSwitchState();
    SysEvent getSwitchState() { return this->constSwitch; };
    void handleEvent(SysEvent);
};

#endif