#include "Particle.h"
#include "enums.hpp"


// // // #define LOGGING_CO
#define VERIOSN                 1
#if VERIOSN == 1
    #define CO_PIN              A0
#else
    #define CO_PIN              A3
#endif
#define HIGHEST_VAL             50//0.4
#define ME2_CO_MAX_OUTPUT_VALUE 50
#define ME2_CO_SAMPLE_TIME      5000
#define CO_MV_PER_PPM           1.0     // mV per ppm — adjust based on sensor sensitivity and load resistor

class ME2_CO
{
private:
    float co;
    bool doSetup, setupSucceeded;
    uint8_t setupAttempts;
    unsigned long lastMeasurement;

    SensorErr setup(void);
    SensorErr measure(void);

public:
    ME2_CO();
    void run(void);
    float getCo(void) { return this->co > ME2_CO_MAX_OUTPUT_VALUE ? ME2_CO_MAX_OUTPUT_VALUE : this->co; }
};