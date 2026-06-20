// #include "QuickyMode.h"


// QuickyMode::QuickyMode(SysModesMngr *mmPtr) : mmPtr(mmPtr)
// {
//     this->whoami = SysMode::QUICK_M;
//     this->timer = nullptr;
//     this->period = 1; /*1h*/
//     this->activeNow = false;
// }


// void QuickyMode::enterMode()
// {
//     #ifdef LOGGING_QUICKY
//         Log.trace("[QuickMode::enterMode] - Enter quick mode");
//     #endif

//     this->activeNow = true;
//     this->timer->reset();

//     this->mmPtr->getExecComponentsChainCallback()(SysEvent::ENTER_QUICK_M);

//     EepromMngr::set("active-mode", "current", NULL, (int8_t)this->whoami);
//     Publisher::publishEvent('S', "1.1.14", "support.modes", "Enter quick mode");
// }


// void QuickyMode::exitMode()
// {
//     #ifdef LOGGING_QUICKY
//         Log.trace("[QuickMode::exitMode] - Exit quick mode");
//     #endif
    
//     if (this->timer->isActive())
//     {
//         #ifdef LOGGING_QUICKY
//             Log.info("[QuickMode::exitMode] - Need to turn off the QuickMode timer (quicky was cancelled)");
//         #endif
//         this->timer->dispose();
//     }
    
//     Publisher::publishEvent('S', "1.1.15", "support.modes", "Exit quick mode");
// }