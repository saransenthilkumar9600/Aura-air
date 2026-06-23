#include "ConnectivityMngr.h"


ConnectivityMngr *ConnectivityMngr::instance = nullptr;


ConnectivityMngr::ConnectivityMngr() : led(Led::getInstance()), sterio(Sterionizer::getInstance()), uvc(Uvc::getInstance()),
                                       coverMngr(CoverMngr::getInstance()), recovMngr(RecoverMngr::getInstance()), modesMngr(SysModesMngr::getInstance())
{
    this->triedToConnect = false;
    this->wasInListeningModeFlag = false;
    this->turnOffWifiFlag = false;

    this->reconnectTimer = new Timer(RECONNECT_TIMER, &ConnectivityMngr::reconnectCallback, *this, true);
}


ConnectivityMngr& ConnectivityMngr::getInstance()
{
    if (instance == nullptr)
        instance = new ConnectivityMngr();

    return *instance;
}


void ConnectivityMngr::run()
{
    if (!this->triedToConnect && !this->delMfgCreds())
    {
        this->triedToConnect = true;

        if (!this->connectivityProcedure()) return;
        
        if (this->recovMngr.getPowerDownResetOccurred())
        {
            this->recovMngr.setPowerDownResetOccurred(false);
            delay(1000);
            this->modesMngr.restoreScheduler();
        }
    }

    if (this->turnOffWifiFlag)
    {
        #ifdef LOGGING_CONNECTIVITYMNGR
            Log.trace("[ConnectivityMngr::run] - The device lost the Cloud connection, turnning off WiFi module");
        #endif
        this->turnOffWifiFlag = false;
        Particle.disconnect();
        WiFi.off();
        this->reconnectTimer->reset();
    }

    this->inspectListeningMode();
}


bool ConnectivityMngr::delMfgCreds()
{
    WiFiAccessPoint ap[5];
    uint8_t numOfCreds = WiFi.getCredentials(ap, 5);
    if (numOfCreds == 1 && String(ap[0].ssid) == "AuraMfgTstWF")
    {
        #ifdef LOGGING_CONNECTIVITYMNGR
            Log.trace("[ConnectivityMngr::delMfgCreds] - Deleteing all WiFi creds");
        #endif
        WiFi.clearCredentials();
        return true;
    }

    #ifdef LOGGING_CONNECTIVITYMNGR
        Log.info("[ConnectivityMngr::delMfgCreds] - No need to delete WiFi creds");
    #endif
    return false;
}


bool ConnectivityMngr::connectivityProcedure()
{
    Particle.variable("SSID", this->cloudVarSsid);
    Particle.variable("EEPROMContent", this->cloudVarEepromConent);
    // The following Cloud Variables does not work perfect (NOT BECAUSE OF ME)
    // Particle.variable("sterionizerState", this->sterio.getSterioState());
    // Particle.variable("uvcState", this->uvc.getUvcState());

    if (WiFi.hasCredentials())
    {
        #ifdef LOGGING_CONNECTIVITYMNGR
            Log.trace("[ConnectivityMngr::connectivityProcedure] - Attempt to connect to a known WiFi creds");
        #endif
        Particle.connect();
        if (waitFor(Particle.connected, 60000))  // Waiting 60 seconds to establish connection
        {
            #ifdef LOGGING_CONNECTIVITYMNGR
                Log.trace("[ConnectivityMngr::connectivityProcedure] - Success to connect to the WiFi and Cloud");
            #endif
            this->reconnectTimer->stop();
            this->cloudVarSsid = String(WiFi.SSID());

            // clear publish queue memory in case of reconnect
            Publisher::clearMemory();

            /*  When the device entered to listening mode manually, we return the the device into auto mode.
                If this is the case, after the user set new wifi creds and the device successeded to connect with tham to internet,
                we want to restore the user defined modes */
            if (this->wasInListeningModeFlag)
            {
                #ifdef LOGGING_CONNECTIVITYMNGR
                    Log.info("[ConnectivityMngr::connectivityProcedure] - Was in Listening Mode. Restoring defined system modes and scheduler");
                #endif
                this->wasInListeningModeFlag = false;
                this->modesMngr.activateDefaultMode();
                this->modesMngr.restoreScheduler();
            }
        }
        else
        {
            if (this->wasInListeningModeFlag)
            {
                #ifdef LOGGING_CONNECTIVITYMNGR
                    Log.trace("[ConnectivityMngr::connectivityProcedure] - Failed to connect to WiFi or Cloud after exiting from Listening Mode");
                #endif
                this->wasInListeningModeFlag = false;
                WiFi.listen();
                return false;
            }

            #ifdef LOGGING_CONNECTIVITYMNGR
                Log.trace("[ConnectivityMngr::connectivityProcedure] - Failed to connect to WiFi or Cloud");
            #endif
            if (!WiFi.listening())
            {
                #ifdef LOGGING_CONNECTIVITYMNGR
                    Log.trace("[ConnectivityMngr::connectivityProcedure] - Turning off the Wi-Fi module");
                #endif
                WiFi.off();
            }

            this->reconnectTimer->reset();
        }
    }
    else
    {
        #ifdef LOGGING_CONNECTIVITYMNGR
            Log.trace("[ConnectivityMngr::connectivityProcedure] - No WiFi creds");
        #endif
        WiFi.listen();
        return false;
    }

    this->led.replaceIndication();
    return true;
}


void ConnectivityMngr::inspectListeningMode()
{
    if (WiFi.listening() && !this->wasInListeningModeFlag)          // If the user entered to LM by Clicking the Setup button
    {
        this->wasInListeningModeFlag = true;
        #ifdef LOGGING_CONNECTIVITYMNGR
            Log.info("[ConnectivityMngr::inspectListeningMode] - Enter Listening Mode");
        #endif

        /*if (this->modesMngr.getQuickyMode() != nullptr)
        {
            this->modesMngr.delQuickMode();
        }*/

        this->led.handleEvent(SysEvent::ENTER_LISTENING_M);     // This will trigger the chain of responsabilities
    }
    else if (!WiFi.listening() && this->wasInListeningModeFlag)
    {
        #ifdef LOGGING_CONNECTIVITYMNGR
            Log.info("[ConnectivityMngr::inspectListeningMode] - Exit Listening Mode");
        #endif
        this->triedToConnect = false;
    }
}


void ConnectivityMngr::removeCreds()
{
    if (WiFi.clearCredentials())
    {
        this->triedToConnect = false;
        Publisher::publishEvent('S', "1.8.1", "support.creds", "Wi-Fi creds removed successfully");
    }
}


// This function recieive string that represent AP name and password seperated by tilda ('~') sign.
// The function seperates the strings into received array variable
void extractCreds(String rawCreds, char creds[2][25])
{
    char spliter = '~';

    for (uint8_t i = 0; i < rawCreds.length(); i++)
    {
        if (rawCreds.charAt(i) != spliter)
        {
            creds[0][i] = rawCreds.charAt(i);
        }
        else if (rawCreds.charAt(i) == spliter)
        {
            rawCreds = rawCreds.remove(0, i+1);
            creds[0][i] = '\0';
            break;
        }
    }

    for (uint8_t i=0; i<rawCreds.length(); i++)
    {
        creds[1][i] = rawCreds.charAt(i);
    }
    creds[1][rawCreds.length()] = '\0';
}


void ConnectivityMngr::setNewCreds(String rawCreds)
{
    WiFiAccessPoint ap[5];
    char creds[2][25];

    extractCreds(rawCreds, creds);
    const char *ssid = creds[0];
    const char *password = creds[1];

    WiFi.on();
    if(WiFi.setCredentials(ssid, password))
    {
        Publisher::publishEvent('S', "1.8.3", "support.creds", "Wi-Fi creds was set successfully");

        Particle.disconnect();
        delay(1500);
        if (Particle.disconnected())
        {
            WiFi.off();
            this->triedToConnect = false;
        }
    }
    delay(20);
}


void ConnectivityMngr::printCreds()
{
    WiFiAccessPoint ap[5];
    uint8_t numOfCredentials = WiFi.getCredentials(ap, 5);

    String str = "";
    for(uint8_t i=0; i<numOfCredentials; i++)
    {
        str += String(i+1) + ") " + String(ap[i].ssid) + " ";
    }

    Particle.publish("internal_support", str);
}


void ConnectivityMngr::setEepromContent()
{
    this->cloudVarEepromConent = EepromMngr::printEeprom();
}


/**
 * Timer callback
 */
void ConnectivityMngr::reconnectCallback()
{
    #ifdef LOGGING_CONNECTIVITYMNGR
        Log.trace("[ConnectivityMngr::reconnectCallback] - Attemp to reconnect...");
    #endif
    this->reconnectTimer->stop();
    WiFi.on();
    this->triedToConnect = false;
}