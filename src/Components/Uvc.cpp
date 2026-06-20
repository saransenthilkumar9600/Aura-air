#include "Uvc.h"


Uvc *Uvc::instance = nullptr;


Uvc::Uvc()
{
    this->constSwitch = SysEvent::UVC_AUTO;

    this->inHighSysMode = false;
    this->coverOpen = false;

    this->uvcState = (bool)PinState::LOW;
}


Uvc& Uvc::getInstance()
{
    if (instance == nullptr)
        instance = new Uvc();

    return *instance;
}


void Uvc::setup()
{
    pinMode(UVC_PIN, OUTPUT);

    digitalWrite(UVC_PIN, LOW);
}


void Uvc::restoreSwitchState()
{
    #ifdef LOGGING_UVC
        Log.trace("[Uvc::restoreSwitchState] - Restore the Uvc switch state...");
    #endif
    uint8_t switchState;
    EepromMngr::get("uvc-switch", NULL, NULL, &switchState);
    if ((SysEvent)switchState == SysEvent::UVC_CONST_OFF || (SysEvent)switchState == SysEvent::UVC_CONST_ON)
    {
        #ifdef LOGGING_UVC
            Log.trace("[Uvc::restoreSwitchState] - Apply restored switch state");
        #endif
        this->handleEvent((SysEvent)switchState);
        return;
    }

    #ifdef LOGGING_UVC
        Log.trace("[Uvc::restoreSwitchState] - Nothing to apply");
    #endif
}


void Uvc::handleEvent(SysEvent e)
{
    switch(e)
    {
        case SysEvent::UVC_CONST_ON:
        {
            this->constSwitch = e;
            EepromMngr::set("uvc-switch", NULL, NULL, (uint8_t)e);
            Publisher::publishEvent('S', "1.5.4", "support.uvc", "UVC turned on");
            if (!this->coverOpen)
            {
                this->uvcState = (bool)PinState::HIGH;
                digitalWrite(UVC_PIN, (PinState)this->uvcState);
            }
            break;
        }
        case SysEvent::UVC_CONST_OFF:
        {
            this->constSwitch = e;
            EepromMngr::set("uvc-switch", NULL, NULL, (uint8_t)e);
            Publisher::publishEvent('S', "1.5.5", "support.uvc", "UVC turned off");
            this->uvcState = (bool)PinState::LOW;
            digitalWrite(UVC_PIN, (PinState)this->uvcState);
            break;
        }
        case SysEvent::UVC_AUTO:
        {
            this->constSwitch = e;
            EepromMngr::set("uvc-switch", NULL, NULL, (uint8_t)e);
            Publisher::publishEvent('S', "1.5.8", "support.uvc", "UVC turned auto");
            break;
        }
        case SysEvent::AQI_LESS_50:
        case SysEvent::AQI_LESS_100:
        {
            if (this->constSwitch == SysEvent::UVC_AUTO && !this->inHighSysMode && !this->coverOpen)
            {
                this->uvcState = (bool)PinState::LOW;
                digitalWrite(UVC_PIN, (PinState)this->uvcState);
            }
            break;
        }
        case SysEvent::AQI_LESS_200:
        case SysEvent::AQI_LESS_500:
        {
            if (this->constSwitch == SysEvent::UVC_AUTO && !this->inHighSysMode && !this->coverOpen)
            {
                this->uvcState = (bool)PinState::HIGH;
                digitalWrite(UVC_PIN, (PinState)this->uvcState);
            }
            break;
        }
        case SysEvent::ENTER_HIGH_M:
        case SysEvent::ENTER_MANUAL_3_M:
        {
            this->inHighSysMode = true;
            if (this->constSwitch == SysEvent::UVC_AUTO && !this->coverOpen)
            {
                this->uvcState = (bool)PinState::HIGH;
                digitalWrite(UVC_PIN, (PinState)this->uvcState);
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
            if (this->constSwitch == SysEvent::UVC_AUTO)
            {
                this->uvcState = (bool)PinState::LOW;
                digitalWrite(UVC_PIN, (PinState)this->uvcState);
            }
            break;
        }
        case SysEvent::COVER_OPEN:
        {
            this->coverOpen = true;
            this->uvcState = (bool)PinState::LOW;
            digitalWrite(UVC_PIN, (PinState)this->uvcState);
            break;
        }
        case SysEvent::COVER_CLOSE:
        {
            coverOpen = false;
            if (this->constSwitch == SysEvent::UVC_CONST_ON || this->inHighSysMode)
            {
                this->uvcState = (bool)PinState::HIGH;
                digitalWrite(UVC_PIN, (PinState)this->uvcState);
            }
            break;
        }
        default:
            break;
    }

    Components::handleEvent(e);
}