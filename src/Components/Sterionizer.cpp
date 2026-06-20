#include "Sterionizer.h"


Sterionizer *Sterionizer::instance = nullptr;


Sterionizer::Sterionizer()
{
    this->constSwitch = SysEvent::STERIO_AUTO;
    
    this->inHighSysMode = false;
    this->coverOpen = false;

    this->sterioState = (bool)PinState::LOW;

    this->lastSterioInspection = 0;
}


Sterionizer& Sterionizer::getInstance()
{
    if (instance == nullptr)
        instance = new Sterionizer();

    return *instance;
}


void Sterionizer::setup()
{
    pinMode(STERIONIZER_PIN, OUTPUT);
    pinMode(STERIONIZER_DIAGNOSTIC, INPUT);

    digitalWrite(STERIONIZER_PIN, LOW);
}


void Sterionizer::restoreSwitchState()
{
    #ifdef LOGGING_STERIO
        Log.trace("[Sterionizer::restoreSwitchState] - Restore the Sterionizer switch state...");
    #endif
    uint8_t switchState;
    EepromMngr::get("sterio-switch", NULL, NULL, &switchState);
    if ((SysEvent)switchState == SysEvent::STERIO_CONST_OFF || (SysEvent)switchState == SysEvent::STERIO_CONST_ON)
    {
        #ifdef LOGGING_STERIO
            Log.trace("[Sterionizer::restoreSwitchState] - Apply restored switch state");
        #endif
        this->handleEvent((SysEvent)switchState);
        return;
    }

    #ifdef LOGGING_STERIO
        Log.trace("[Sterionizer::restoreSwitchState] - Nothing to apply");
    #endif
}


uint8_t Sterionizer::getSterioDiagnos()
{
    bool closeAfterDiagnos = false;
    if (!this->sterioState)
    {
        closeAfterDiagnos = true;
        digitalWrite(STERIONIZER_PIN, HIGH);
        delay(10);
    }

    bool result = false;
    for(uint8_t i=0; i<15; i++)
        result = result || (bool)digitalRead(STERIONIZER_DIAGNOSTIC);

    if (closeAfterDiagnos)
        digitalWrite(STERIONIZER_PIN, LOW);

    return (uint8_t)result;
}


void Sterionizer::handleEvent(SysEvent e)
{
    switch(e)
    {
        case SysEvent::STERIO_CONST_ON:
        {
            this->constSwitch = e;
            EepromMngr::set("sterio-switch", NULL, NULL, (uint8_t)e);
            Publisher::publishEvent('S', "1.5.2", "support.sterionizer", "Streionizer turned on");
            if (!this->coverOpen)
            {
                this->sterioState = (bool)PinState::HIGH;
                digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            }
            break;
        }
        case SysEvent::STERIO_CONST_OFF:
        {
            this->constSwitch = e;
            EepromMngr::set("sterio-switch", NULL, NULL, (uint8_t)e);
            Publisher::publishEvent('S', "1.5.3", "support.sterionizer", "Streionizer turned off");
            this->sterioState = (bool)PinState::LOW;
            digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            break;
        }
        case SysEvent::STERIO_AUTO:
        {
            this->constSwitch = e;
            EepromMngr::set("sterio-switch", NULL, NULL, (uint8_t)e);
            Publisher::publishEvent('S', "1.5.9", "support.sterionizer", "Streionizer turned auto");
            break;
        }
        case SysEvent::AQI_LESS_50:
        case SysEvent::AQI_LESS_100:
        {
            if (this->constSwitch == SysEvent::STERIO_AUTO && !this->inHighSysMode && !this->coverOpen)
            {
                this->sterioState = (bool)PinState::LOW;
                digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            }
            break;
        }
        case SysEvent::AQI_LESS_200:
        case SysEvent::AQI_LESS_500:
        {
            if (this->constSwitch == SysEvent::STERIO_AUTO && !this->inHighSysMode && !this->coverOpen)
            {
                this->sterioState = (bool)PinState::HIGH;
                digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            }
            break;
        }
        case SysEvent::ENTER_HIGH_M:
        case SysEvent::ENTER_MANUAL_3_M:
        {
            this->inHighSysMode = true;
            if (this->constSwitch == SysEvent::STERIO_AUTO && !this->coverOpen)
            {
                this->sterioState = (bool)PinState::HIGH;
                digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            }
            break;
        }
        case SysEvent::ENTER_AUTO_M:
        case SysEvent::ENTER_LOW_M:
        case SysEvent::ENTER_SILENT_M:
        case SysEvent::ENTER_NIGHT_M:
        case SysEvent::ENTER_OFF_M:
        case SysEvent::ENTER_MANUAL_1_M:
        case SysEvent::ENTER_MANUAL_2_M:
        {
            this->inHighSysMode = false;
            if (this->constSwitch == SysEvent::STERIO_AUTO)
            {
                this->sterioState = (bool)PinState::LOW;
                digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            }
            break;
        }
        case SysEvent::COVER_OPEN:
        {
            this->coverOpen = true;
            this->sterioState = (bool)PinState::LOW;
            digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            break;
        }
        case SysEvent::COVER_CLOSE:
        {
            coverOpen = false;
            if (this->constSwitch == SysEvent::STERIO_CONST_ON || this->inHighSysMode)
            {
                this->sterioState = (bool)PinState::HIGH;
                digitalWrite(STERIONIZER_PIN, (PinState)this->sterioState);
            }
            break;
        }
        default:
            break;
    }

    if (millis() - this->lastSterioInspection > STERIO_INSPEC_INTERVAL)
    {
        if (this->getSterioDiagnos() == 0)
            Publisher::publishEvent('S', "1.5.1", "support.sterionizer", "Sterionizer doesn't work properly");

        this->lastSterioInspection = millis();
    }

    Components::handleEvent(e);
}