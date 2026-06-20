#ifndef COVERMNGR_H
#define COVERMNGR_H


#include "enums.hpp"
#include <functional>
#include "Particle.h"
#include "Publisher/Publisher.h"


// #define LOGGING_COVERMNGR
#define HW_VER 1
#if HW_VER == 1
    #define HALLEFFECT_PIN P1S4
#endif


typedef std::function<void(SysEvent)> callbackFunc;


class CoverMngr
{
private:
    static CoverMngr *instance;
    SysEvent coverState;
    bool _coverState;
    callbackFunc execComponentsChainCallback;

    CoverMngr();

public:
    static CoverMngr& getInstance();
    static void setup();
    void lateSetup(callbackFunc);
    bool getCoverState() { return this->_coverState; }
    void inspectCoverState();
};

#endif