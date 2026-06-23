#include "Particle.h"
#include "Publisher/Publisher.h"


#define START_BYTE 0xFF
#define NAME_CODE 0x18


enum SensorState
{
    pendingFF,
    pending18,
    reading
};


class ZPH02
{
private:
    SensorState state;
    uint8_t index, counter;
    uint8_t incomingData[9];
    int16_t pm2, pm10;

    bool dataValidation();

public:
    ZPH02();
    void run();
    int16_t getPm2() { return this->pm2; }
    int16_t getPm10() { return this->pm10; }
};