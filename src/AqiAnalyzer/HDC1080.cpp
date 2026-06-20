#include "HDC1080.h"


HDC1080::HDC1080()
{
    this->temperature = -1;
    this->humidity = -1;
    this->doSetup = true;
    this->setupAttempts = 3;
    this->setupSucceeded = false;
    this->lastMeasurement = 0;
}


SensorErr HDC1080::measure(void)
{
    Wire.beginTransmission(HDC1080_ADDRS);
    Wire.write(0x00);       // Move the pointer register to TEMP register
    uint8_t res = Wire.endTransmission();
    if (res != 0)
    {
        #ifdef LOGGING_HDC1080
            Log.error("[HDC1080::measure] - I2C endTransmission returns: %d", res);
        #endif
        return SensorErr::I2C_END_TRANSMISSION_ERR;
    }

    delay(HDC1080_DELAY);

    res = Wire.requestFrom(HDC1080_ADDRS, 4);
    if (res != 4)
    {
        #ifdef LOGGING_HDC1080
            Log.error("[HDC1080::measure] - I2C requestFrom returns: %d", res);
        #endif
        return SensorErr::I2C_REQUEST_FROM_ERR;
    }

    uint16_t tmpTemp = (((unsigned int)Wire.read() << 8 | Wire.read()));
    this->temperature = (float)(tmpTemp) / 65536 * 165 - 40;

    uint16_t tmpHumid = (((unsigned int)Wire.read() << 8 | Wire.read()));
    this->humidity = (float)(tmpHumid) / (65536) * 100;

    return SensorErr::NO_ERR;
}


SensorErr HDC1080::setup(void)
{
    Wire.beginTransmission(HDC1080_ADDRS);
    Wire.write(0x02);       // Move the pointer register to configuration register
    Wire.write(0x10); 
    uint8_t res = Wire.endTransmission();
    if (res != 0)
    {
        #ifdef LOGGING_HDC1080
            Log.error("[HDC1080::setup] - I2C endTransmission returns: %d",res);
        #endif
        return SensorErr::I2C_END_TRANSMISSION_ERR;
    }

    delay(HDC1080_DELAY);

    return SensorErr::NO_ERR;
}


void HDC1080::run(void)
{
    if (this->doSetup && this->setupAttempts > 0)
    {
        if (this->setup() == SensorErr::NO_ERR)     // If setup succeeded
        {
            this->setupSucceeded = true;
            this->doSetup = false;

            #ifdef LOGGING_HDC1080
                Log.trace("[HDC1080::run] - Setup succeeded");
            #endif
        }
        else if (--this->setupAttempts == 0)        // If there are no more setup attempts
        {
            this->doSetup = false;
            
            Publisher::publishEvent('S', "1.7.1", "support.hdc1080", "HDC1080 setup failed");
            #ifdef LOGGING_HDC1080
                Log.error("[HDC1080::run] - Setup failed and there are no more attempts to do a setup");
            #endif
        }
    }

    if (this->setupSucceeded && (millis() - this->lastMeasurement >= HDC1080_SAMPLE_TIME || this->lastMeasurement == 0))
    {
        if (this->measure() != SensorErr::NO_ERR)
        {
            this->temperature = -1;
            this->humidity = -1;

            this->lastMeasurement = millis();
            
		    Publisher::publishEvent('S', "1.7.2", "support.hdc1080", "HDC1080 measurement failed");
            #ifdef LOGGING_HDC1080
                Log.error("[HDC1080::run] - Measurement failed");
            #endif
            return;
        }

        #ifdef LOGGING_HDC1080
	        Log.info("[HDC1080::run] - hdc1080 measured temperature: %.1f", this->getTemperature());
		    Log.info("[HDC1080::run] - hdc1080 measured humidity: %.1f", this->getHumidity());
	    #endif

        this->lastMeasurement = millis();
    }
}