#include "Components.h"


Components::Components()
{
    this->nextComp = nullptr;
}


void Components::setNextHandler(Components *comp)
{
    this->nextComp = comp;
}


void Components::addNextHandler(Components *comp)
{
    if (this->nextComp != nullptr)
        this->nextComp->addNextHandler(comp);
    else
        this->nextComp = comp;
}


void Components::handleEvent(SysEvent e)
{
    if (this->nextComp != nullptr)
        this->nextComp->handleEvent(e);
}