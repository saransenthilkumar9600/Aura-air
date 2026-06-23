#ifndef EEPROMMANAGER_H
#define EEPROMMANAGER_H


#include "enums.hpp"
#include "Particle.h"
#include "Publisher/Publisher.h"
#include "ArduinoJson-v6.17.2.h"


// #define LOGGING_EEPROM
#define EEPROM_IS_DEFINED_ADDRS  1
#define EEPROM_DATA_OBJ_ADDRS    2
#define EEPROM_REINIT_FLAG_ADDRS 2043  // Max is 2047. range 2042-2047
#define SAVE_PREV_EEPROM_DATA    true
#define JSON_DOC_SIZE            1200
#define EEPROM_CONTENT_SIZE      900


struct EepromContent
{
    char jDoc[EEPROM_CONTENT_SIZE];
};


#if SAVE_PREV_EEPROM_DATA == true
    struct TmpSchedulerData
    {
        int8_t whoami;
        char startTime[6];
        float period;

        TmpSchedulerData() : whoami((int8_t)SysMode::NOT_INIT), period(0.0) {}
    };

    struct LedSchedulerTmp
    {
        bool enabled;
        char startTime[6];
        char endTime[6];
        bool state;

        LedSchedulerTmp() : enabled(false), state(true) { startTime[0] = '\0'; endTime[0] = '\0'; }
    };

    struct EepromTmpSavedContent
    {
        uint8_t schedSize;
        uint8_t schedPosition;
        TmpSchedulerData *schedulerDataArr;
        int8_t defaultMode;
        int8_t currentMode;
        bool autoSilentSwitch;
        bool ledSwitch;
        int8_t timezone;
        bool fanSwitch;
        LedSchedulerTmp ledScheduler;

        EepromTmpSavedContent() : schedSize((uint8_t)SchedSize::EMPTY), schedPosition((uint8_t)SchedPosition::FIRST), 
                                defaultMode((int8_t)SysMode::LOW_M), currentMode((int8_t)SysMode::LOW_M), ledSwitch(true), timezone(NULL), fanSwitch(true), autoSilentSwitch(false) {}
    };
#endif


class EepromMngr
{
private:
    #if SAVE_PREV_EEPROM_DATA == true
        EepromTmpSavedContent tmpContent;

        void saveData();
    #endif
    bool isEepromInit();

public:
    static void initEeprom();
    static String printEeprom();
    static void clearEeprom();
    static int getMemUsage();

    static void set(const char*, const char*, bool);
    static void get(const char*, const char*, bool*);

    static void set(const char*, const char*, const char*, bool);
    static void get(const char*, const char*, const char*, bool*);

    static void set(const char*, const char*, const char*, uint8_t);
    static void get(const char*, const char*, const char*, uint8_t*);

    static void set(const char*, const char*, const char*, int8_t);
    static void get(const char*, const char*, const char*, int8_t*);

    static void set(const char*, const char*, const char*, int16_t);
    static void get(const char*, const char*, const char* , int16_t*);

    static void set(const char*, const char*, const char*, float);
    static void get(const char*, const char*, const char*, float*);

    static void set(const char*, const char*, const char*, const char*);
    static void get(const char*, const char*, const char*, char*);

    static void set(const char*, const char*, const char*, unsigned long);
    static void get(const char*, const char*, const char*, unsigned long*);
};

#endif