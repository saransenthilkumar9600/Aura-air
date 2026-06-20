#include "Particle.h"
#include "enums.hpp"
#include "math.h"
#include "Publisher/Publisher.h"


// // // #define LOGGING_SGP30
#define SGP30_ADDRS             0x58 
#define SGP30_DELAY             12
#define SGP30_SELF_TEST_DELAY   250
#define SGP30_SAMPLE_TIME       0
#define SGP30_MAX_OUTPUT_VALUE  5000    


class SGP30 
{
private:
    const uint8_t self_test[2]                 = {0x20, 0x32};
    const uint8_t set_humidity_compensation[2] = {0x20, 0x61};
    const uint8_t init_air_quality[2]          = {0x20, 0x03};
    const uint8_t measure_air_quality[2]       = {0x20, 0x08};
    int16_t voc, eco2;
    bool doSetup, setupSucceeded;
    uint8_t setupAttempts;
    unsigned long lastMeasurement;

    uint8_t crc8(uint16_t);
    SensorErr measure(float, float);
    SensorErr selfTest(void);
    SensorErr setup(void);

public:
    SGP30();
    void run(float, float);
    int16_t getEco2(void) 
    {
        if (this->eco2 < -1 || this->eco2 > SGP30_MAX_OUTPUT_VALUE)
            return SGP30_MAX_OUTPUT_VALUE; 
        else 
            return this->eco2; 
    }
    int16_t getTvoc(void) 
    {
        if (this->voc < -1 || this->voc > SGP30_MAX_OUTPUT_VALUE)
            return SGP30_MAX_OUTPUT_VALUE;
        else
            return this->voc;
    }
};
