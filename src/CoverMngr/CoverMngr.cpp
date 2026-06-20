#include "CoverMngr.h"


CoverMngr *CoverMngr::instance = nullptr;


CoverMngr::CoverMngr()
{
    this->coverState = SysEvent::COVER_CLOSE;
    this->_coverState = false;
}


CoverMngr& CoverMngr::getInstance()
{
    if (instance == nullptr)
        instance = new CoverMngr();

    return *instance;
}


void CoverMngr::setup()
{
    pinMode(HALLEFFECT_PIN, INPUT);
}


void CoverMngr::lateSetup(callbackFunc execComponentsChainCallback)
{
    this->execComponentsChainCallback = execComponentsChainCallback;
}


void CoverMngr::inspectCoverState()
{
    if (digitalRead(HALLEFFECT_PIN) == PinState::HIGH && this->coverState == SysEvent::COVER_CLOSE)
    {
        this->coverState = SysEvent::COVER_OPEN;
        this->_coverState = true;
        this->execComponentsChainCallback(this->coverState);
        Publisher::publishEvent('S', "1.10.1", "support.cover", "The front cover opened");
    }
    else if (digitalRead(HALLEFFECT_PIN) == PinState::LOW && this->coverState == SysEvent::COVER_OPEN)
    {
        this->coverState = SysEvent::COVER_CLOSE;
        this->_coverState = false;
        this->execComponentsChainCallback(this->coverState);
        Publisher::publishEvent('S', "1.10.2", "support.cover", "The front cover closed");
    }
    
    Particle.variable("coverState", this->_coverState);
}