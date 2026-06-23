#include "RecoverMngr.h"


RecoverMngr *RecoverMngr::instance = nullptr;


RecoverMngr::RecoverMngr()
{
    this->powerDownResetOccurred = false;
}


RecoverMngr &RecoverMngr::getInstance()
{
    if (instance == nullptr)
        instance = new RecoverMngr();

    return *instance;
}


void RecoverMngr::recover()
{
    // Collect last reset reason if its Panic or User reset
    if (System.resetReason() == RESET_REASON_PANIC)
    {
        #ifdef LOGGING_RECOV
            Log.trace("[RecoverMngr::recover] - A panic reset occuered");
        #endif
        this->panicRecover(); // Mechanism for recovery from panic resets
    }
    else if (System.resetReason() == RESET_REASON_USER)
    {
        #ifdef LOGGING_RECOV
            Log.trace("[RecoverMngr::recover] - A user reset occuered");
        #endif
        this->resetRecover(); // Mechanism for recovery from raw user resets
    }
    else if (System.resetReason() == RESET_REASON_POWER_DOWN)
    {
        #ifdef LOGGING_RECOV
            Log.trace("[RecoverMngr::recover] - A power down occuered");
        #endif
        this->powerDownResetOccurred = true;
    }
    else if (System.resetReason() == RESET_REASON_UPDATE)
    {
        #ifdef LOGGING_RECOV
            Log.trace("[RecoverMngr::recover] - A SW update occuered");
        #endif
    }
}

void RecoverMngr::panicRecover()
{
    uint8_t currPanicCounter;
    unsigned long lastPanic;

    EepromMngr::get("panic-reset", "panic-counter", NULL, &currPanicCounter);
    EepromMngr::get("panic-reset", "last-panic-event", NULL, &lastPanic);

    if (currPanicCounter == 0)
    {
        EepromMngr::set("panic-reset", "panic-counter", NULL, (uint8_t)1);
        EepromMngr::set("panic-reset", "last-panic-event", NULL, (unsigned long)Time.now());
        #ifdef LOGGING_RECOV 
            Log.info("[Recover::panicRecover] - Panic events counter: %d", currPanicCounter + 1);
        #endif
    }
    else // If success, check if it's the 5th time. If yes, clear the panic reset obj and enter to safe mode.
    {
        #ifdef LOGGING_RECOV
            Log.info("[Recover::panicRecover] - Panic events counter: %d", currPanicCounter);
        #endif

        if (currPanicCounter >= 5)
        {
            EepromMngr::set("panic-reset", "panic-counter", NULL, (uint8_t)0);
            EepromMngr::set("panic-reset", "last-panic-event", NULL, (unsigned long)0);
            #ifdef LOGGING_RECOV
                Log.trace("[Recover::panicRecover] - 5 panic reset events occuered. Entering to Safe Mode...");
            #endif
            delay(5);
            System.enterSafeMode();
        }
        else // If no, increase the counter and update the 'lastPanic' time.
        {
            if (Time.now() - lastPanic <= 600) // 600 it's 10 minuts
            {
                EepromMngr::set("panic-reset", "panic-counter", NULL, (uint8_t)(currPanicCounter + 1));
                EepromMngr::set("panic-reset", "last-panic-event", NULL, (unsigned long)Time.now());
                #ifdef LOGGING_RECOV
                    Log.trace("[Recover::panicRecover] - The current panic reset event saved to EEPROM");
                #endif
            }
        }
    }
}


void RecoverMngr::resetRecover()
{
    uint8_t currResetCounter;
    unsigned long lastReset;

    EepromMngr::get("user-reset", "reset-counter", NULL, &currResetCounter);
    EepromMngr::get("user-reset", "last-reset-event", NULL, &lastReset);

    if (currResetCounter == 0)
    {
        EepromMngr::set("user-reset", "reset-counter", NULL, (uint8_t)1);
        EepromMngr::set("user-reset", "last-reset-event", NULL, (unsigned long)Time.now());
        #ifdef LOGGING_RECOV
            Log.info("[Recover::resetRecover] - User reset events counter: %d", currResetCounter + 1);
        #endif
    }
    else // If success, check if it's the 5th time. If yes, clear the panic reset obj and enter to safe mode.
    {
        #ifdef LOGGING_RECOV
            Log.info("[Recover::resetRecover] - User reset events counter: %d", currResetCounter);
        #endif

        if (currResetCounter >= 5)
        {
            EepromMngr::set("user-reset", "reset-counter", NULL, (uint8_t)0);
            EepromMngr::set("user-reset", "last-reset-event", NULL, (unsigned long)0);
            #ifdef LOGGING_RECOV
                Log.trace("[Recover::resetRecover] - 5 user reset events occuered. Entering to Safe Mode...");
            #endif
            delay(5);
            System.enterSafeMode();
        }
        else // If no, increase the counter and update the 'lastReset' time.
        {
            if (Time.now() - lastReset <= 120) 
            {
                EepromMngr::set("user-reset", "reset-counter", NULL, (uint8_t)(currResetCounter + 1));
                EepromMngr::set("user-reset", "last-reset-event", NULL, (unsigned long)Time.now());
                #ifdef LOGGING_RECOV
                    Log.trace("[Recover::resetRecover] - The current user reset event saved to EEPROM");
                #endif                
            }
        }
    }
}