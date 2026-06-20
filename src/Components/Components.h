#ifndef COMPONENTS_H
#define COMPONENTS_H


#include "enums.hpp"


class Components
{
private:
    Components *nextComp;

public:
    Components();
    void setNextHandler(Components*);
    void addNextHandler(Components*);
    virtual void handleEvent(SysEvent);
};

#endif