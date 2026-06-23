/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/gilad/Library/Aura/Projects/aura_firmware/src/main.ino"
#include "SysMngr/SysMngr.h"


// #define LOGGING_MAIN
int printEeprom(String data);
int clearEeprom(String data);
int setSystemMode(String data);
int setFanSpeed(String data);
int getFanRpm(String data);
int ledSwitch(String data);
int sterioSwitch(String data);
int getSterioDiagnos(String data);
int uvcSwitch(String data);
int setTimeZone(String data);
int setTime(String data);
int setCreds(String data);
int printCreds(String data);
int hardReset(String data);
int softSafeMode(String data);
int getEepromMemUsage(String data);
int retriveMacAddrs(String data);
void networkStatusHandler(system_event_t e, int param);
void earlySetup();
void setup();
void loop();
#line 5 "/Users/gilad/Library/Aura/Projects/aura_firmware/src/main.ino"
#define HW_VER 1
#define FW_VER "v1.19"


#if HW_VER == 1
    PRODUCT_ID(9494);
#endif
PRODUCT_VERSION(52);
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);
STARTUP(System.enableFeature(FEATURE_RESET_INFO); earlySetup(););

String macAddrs = "";
SerialLogHandler logHandler(LOG_LEVEL_ALL, {{ "app", LOG_LEVEL_ALL }, { "app.network", LOG_LEVEL_ALL } });
SysMngr mngr;
// Related to Quick mode feature
// bool quickModeFlag = false;

int retriveMacAddrs(String data)
{
    byte mac[6];
    WiFi.macAddress(mac);

    for (int i = 0; i < 6; i++)
    {
        macAddrs += String(mac[i]);
        if (i != 5)
        {
            macAddrs += ":";
        }
    }

    return 1;
}

/**
 * This function delegate Particle cloud funciton calls to system mngr which 
 * implements the cloud function
 */
int printEeprom(String data)
{
    return mngr.printEeprom(data);
}


int clearEeprom(String data)
{
    if (data == "true")
    {
        EepromMngr::clearEeprom();
        return 1;
    }

    return 0;
}


/**
 * This function delegate Particle cloud funciton calls to system mngr which 
 * implements the cloud function
 */
int setSystemMode(String data)
{
    return mngr.setSystemMode(data);
}


int setFanSpeed(String data)
{
    return mngr.setFanSpeed(data);
}


int getFanRpm(String data)
{
    return mngr.getFanRpm(data);
}


/**
 * This function delegate Particle cloud funciton calls to system mngr which 
 * implements the cloud function
 */
int ledSwitch(String data)
{
    return mngr.ledSwitch(data);
}


/**
 * This function delegate Particle cloud funciton calls to system mngr which 
 * implements the cloud function
 */
int sterioSwitch(String data)
{
    return mngr.sterioSwitch(data);
}


int getSterioDiagnos(String data)
{
    return mngr.getSterioDiagnos(data);
}


/**
 * This function delegate Particle cloud funciton calls to system mngr which 
 * implements the cloud function
 */
int uvcSwitch(String data)
{
    return mngr.uvcSwitch(data);
}


int setTimeZone(String data)
{
    return mngr.setTimeZone(data);
}


int setTime(String data)
{
    Time.setTime((time_t)data.toInt());
    return 1;
}


int setCreds(String data)
{
    return mngr.setCreds(data);
}


int printCreds(String data)
{
    return mngr.printCreds(data);
}


int hardReset(String data)
{
    System.reset();
    return 1;
}


int softSafeMode(String data)
{
    System.enterSafeMode();
    return 1;
}


int getEepromMemUsage(String data)
{
    return EepromMngr::getMemUsage();
}


void networkStatusHandler(system_event_t e, int param)
{
    if (param == network_status_disconnecting)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - Disconnecting from WiFi");
        #endif
        if (!WiFi.listening()) ConnectivityMngr::getInstance().setTurnOffWifiFlag(true);
    }
    else if (param == network_status_disconnected)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - Disconnected from WiFi");    
        #endif
    }
    else if (param == network_status_powering_off)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - Powring off the WiFi module");
        #endif
    }
    else if (param == network_status_off)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - The WiFi module powered off");
        #endif
    }
    else if (param == network_status_powering_on)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - Powring on the WiFi module");
        #endif
    }
    else if (param == network_status_on)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - The WiFi module powered on");
        #endif
    }
    else if (param == network_status_connecting)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - Connecting to WiFi");
        #endif
    }
    else if (param == network_status_connected)
    {
        #ifdef LOGGING_MAIN
            Log.trace("[networkStatusHandler] - Connected to WiFi");
        #endif
    }
}


// Related to Quick mode feature
// void btnClickHandler(system_event_t, int param)
// {
//     if (system_button_clicks(param) == 1) quickModeFlag = true;
// }


void earlySetup()
{
    #ifdef LOGGING_MAIN
        Serial.begin(9600);
    #endif
    Serial1.begin(9600);

    RGB.control(true);              
    RGB.color(0, 0, 0);

    // Setup components early as possible
    Led::setup();
    Fan::setup();
    CoverMngr::setup();
    Sterionizer::setup();
    Uvc::setup();
}


void setup() 
{
    #ifdef LOGGING_MAIN
        delay(3000);
        Log.trace("[setup] - Setup start");
    #endif

    Particle.function("printEEPROM", printEeprom);
    Particle.function("clearEEPROM", clearEeprom);
    Particle.function("getEEPROMMemoryUsage", getEepromMemUsage);
    Particle.function("setSystemMode", setSystemMode);
    Particle.function("setFanSpeed", setFanSpeed);
    Particle.function("getFanRPM", getFanRpm);
    Particle.function("switchLed", ledSwitch);
    Particle.function("switchSterio", sterioSwitch);
    Particle.function("getSterioDiagnos", getSterioDiagnos);
    Particle.function("switchUvc", uvcSwitch);
    Particle.function("setTimeZone", setTimeZone);
    Particle.function("setTime", setTime);
    Particle.function("setCredentials", setCreds);
    Particle.function("printCreds", printCreds);
    Particle.function("reset", hardReset);
    Particle.function("retriveMacAddrs", retriveMacAddrs);

    Particle.variable("macAddrs", macAddrs);
    Particle.variable("FW_VER", FW_VER);

    System.set(SYSTEM_CONFIG_SOFTAP_PREFIX, "Aura");
    System.on(network_status, networkStatusHandler);
    // Related to Quick mode feature
    // System.on(button_final_click, btnClickHandler);
    System.disable(SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS);

    RGB.control(false);

    mngr.setup();

    #ifdef LOGGING_MAIN
        Log.trace("[setup] - Setup done");
    #endif
}


void loop() 
{
    mngr.run();
    Particle.process();

    // Related to Quick mode feature
    // if (quickModeFlag)
    // {
    //     quickModeFlag = false;
    //     setSystemMode("6_on");
    // }
}