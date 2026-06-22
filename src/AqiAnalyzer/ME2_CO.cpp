#include "ME2_CO.h"



ME2_CO::ME2_CO()
{
    this->co = -1.0;
    this->doSetup = true;
    this->setupAttempts = 3;
    this->setupSucceeded = false;
    this->lastMeasurement = 0;
}

SensorErr ME2_CO::measure(void)
{
    int adcRaw = analogRead(CO_PIN);
    
    float mV = (3300 / 4096.0) * adcRaw;
    this->co = mV / CO_MV_PER_PPM;
    return SensorErr::NO_ERR;
}

SensorErr ME2_CO::setup(void)
{
    pinMode(CO_PIN, INPUT);
    return SensorErr::NO_ERR;
}

void ME2_CO::run(void)
{
    if (this->doSetup && this->setupAttempts > 0)
    {
        if (this->setup() == SensorErr::NO_ERR)     // If setup succeeded
        {
            this->setupSucceeded = true;
            this->doSetup = false;

            #ifdef LOGGING_CO
                Log.trace("[ME2_CO::run] - Setup succeeded");
            #endif
        }
        else if (--this->setupAttempts == 0)        // If there are no more setup attempts
        {
            this->doSetup = false;

            #ifdef LOGGING_CO
                Log.error("[ME2_CO::run] - Setup failed and there are no more attempts to do a setup");
            #endif
        }
    }

	if (this->setupSucceeded && (millis() - this->lastMeasurement >= ME2_CO_SAMPLE_TIME || this->lastMeasurement == 0))
	{
        if (this->measure() != SensorErr::NO_ERR)
        {
            this->co = -1;

			this->lastMeasurement = millis();
            
            #ifdef LOGGING_CO
                Log.error("[ME2_CO::run] - Measurement failed");
            #endif
            return;
        }

        #ifdef LOGGING_CO
            Log.info("[ME2_CO::run] - CO measured co: %.1f", this->getCo());
        #endif

		this->lastMeasurement = millis();
    }
}