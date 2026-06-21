#ifndef CONNECTIVITYMNGR_H
#define CONNECTIVITYMNGR_H


#include "Particle.h"
#include "Components/Led.h"
#include "Components/Sterionizer.h"
#include "Components/Uvc.h"
#include "CoverMngr/CoverMngr.h"
#include "RecoverMngr/RecoverMngr.h"
#include "SysModes/SysModesMngr.h"


// #define LOGGING_CONNECTIVITYMNGR
#define RECONNECT_TIMER 120000


class ConnectivityMngr
{
private:
    
    Led &led;
    Sterionizer &sterio;
    Uvc &uvc;
    CoverMngr &coverMngr;
    RecoverMngr &recovMngr;
    SysModesMngr &modesMngr;
    static ConnectivityMngr *instance;
    Timer *reconnectTimer;
    bool wasInListeningModeFlag, triedToConnect, turnOffWifiFlag;
    String cloudVarSsid, cloudVarEepromConent;

    ConnectivityMngr();
    bool delMfgCreds();
    bool connectivityProcedure();
    void reconnectCallback();

public:
    static ConnectivityMngr& getInstance();
    void registerCloudVariables();
    void run();
    void removeCreds();
    void setNewCreds(String);
    void printCreds();
    void inspectListeningMode();
    void setEepromContent();
    void setTurnOffWifiFlag(bool state) { this->turnOffWifiFlag = state; }
};

#endif