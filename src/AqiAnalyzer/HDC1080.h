#include "Particle.h"
#include "enums.hpp"
#include "Publisher/Publisher.h"


// // // #define LOGGING_HDC1080
#define HDC1080_ADDRS 0x40
#define HDC1080_DELAY 25
#define HDC1080_SAMPLE_TIME 0


class HDC1080
{
private:
    float humidity;
    float temperature;
    bool doSetup, setupSucceeded;
    uint8_t setupAttempts;
    unsigned long lastMeasurement;

    SensorErr measure(void);
    SensorErr setup(void);

public:
    HDC1080();
    void run(void);
    float getHumidity(void) { return this->humidity; }
    float getTemperature(void) { return this->temperature; }
};