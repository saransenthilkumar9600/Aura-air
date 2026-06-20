#ifndef RECOVERMNGR_H
#define RECOVERMNGR_H


#include "EepromMngr/EepromMngr.h"


// #define LOGGING_RECOV


class RecoverMngr
{
private:
    static RecoverMngr *instance;
    bool powerDownResetOccurred;

    RecoverMngr();

public:
    static RecoverMngr& getInstance();
    void recover();
    void panicRecover();
    void resetRecover();
    bool getPowerDownResetOccurred() { return this->powerDownResetOccurred; }
    void setPowerDownResetOccurred(bool state) { this->powerDownResetOccurred = state; }
};

#endif