#ifndef MODESMANAGER_H
#define MODESMANAGER_H


#include "Particle.h"
#include "Scheduler.h"
// #include "QuickyMode.h"
#include "Publisher/Publisher.h"
#include "EepromMngr/EepromMngr.h"


// #define LOGGING_SYSMODES
#define NUM_OF_SCHEDULERS 6


typedef std::function<void(SysEvent)> callbackFunc;


class Scheduler;
// class QuickyMode;


class SysModesMngr
{
private:
    static SysModesMngr *instance;
    Scheduler &sched;
    SysMode defaultMode;
    /*QuickyMode *quicky;
    bool quickModeEnds;*/
    callbackFunc execComponentsChainCallback;

    SysModesMngr();

public:
    static SysModesMngr& getInstance();
    void setup(callbackFunc);
    void restoreScheduler();
    void restoreDefaultMode();
    void inspectScheduledModesState();
    SysModeCommStatus setDefaultMode(SysMode);
    SysModeCommStatus resetDefaultMode(SysMode);
    void activateDefaultMode(SysMode mode=SysMode::NOT_INIT);
    SysModeCommStatus defScheduler(SysMode*, char[NUM_OF_SCHEDULERS][6], float*, float*, uint8_t);
    // sSysModeCommStatus defQuickMode();
    SysModeCommStatus delScheduler();
    // void delQuickMode();
    callbackFunc getExecComponentsChainCallback()   { return this->execComponentsChainCallback; }
    SysMode getDefaultMode()                        { return this->defaultMode; } 
    Scheduler& getScheduler()                       { return this->sched; }
    // QuickyMode* getQuickyMode()                     { return this->quicky; }
};

#endif