#include "Components/Led.h"
#include "Components/Fan.h"
#include "Components/Uvc.h"
#include "Components/Sterionizer.h"
#include "CoverMngr/CoverMngr.h"
#include "AqiAnalyzer/AqiAnalyzer.h"
#include "EepromMngr/EepromMngr.h"
#include "SysModes/SysModesMngr.h"
#include "RecoverMngr/RecoverMngr.h"
#include "ConnectivityMngr/ConnectivityMngr.h"


using namespace std::placeholders;


// #define LOGGING_SYSMNGR
#define MAX_SCHED_MODE_INPUT_SIZE   31 //For example: ~3_1_18:00_1.1111111_0.1111111~ 
// #define MAX_SCHED_MODE_INPUT_SIZE   32 //For example: ~3_10_18:00_1.1111111_0.1111111~ 


class SysMngr
{
private:
    Led &led;
    Fan &fan;
    Uvc &uvc;
    Sterionizer &sterio;
    CoverMngr &coverMngr;
    AqiAnalyzer &aqiLyzer;
    SysModesMngr &modesMngr;
    RecoverMngr &recovMngr;
    ConnectivityMngr &connMngr;
    std::function<void(SysEvent)> execComponentsChainCallback;

    bool parseSingleSchedMode(String, char*, float*, float*);

public:
    SysMngr();
    void setup();
    void registerCloudVariables();
    void run();
    void execComponentsChain(SysEvent);
    void btnCycleModes();
    bool isLedBlinking() { return this->led.isBlinking(); }
    int printEeprom(String);
    int getModeFromString(String);
    int setSystemMode(String);
    int setFanSpeed(String);
    int getFanRpm(String);
    int ledSwitch(String);
    int setLedScheduler(String);
    int sterioSwitch(String);
    int getSterioDiagnos(String);
    int uvcSwitch(String);
    int setTimeZone(String);
    int setCreds(String);
    int printCreds(String);
};