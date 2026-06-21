#include "SGP30.h"


SGP30::SGP30() 
{
	this->voc = -1;
	this->eco2 = -1;
	this->doSetup = true;
	this->setupAttempts = 3;
	this->setupSucceeded = false;
	this->lastMeasurement = 0;
	this->setupTimestamp = 0;
}


uint8_t SGP30::crc8(uint16_t data)
{
	uint8_t crc = 0xFF; //Init with 0xFF

	crc ^= (data >> 8); // XOR-in the first input byte
	for (uint8_t i = 0; i < 8; i++)
	{
		if ((crc & 0x80) != 0)
		{
			crc = (uint8_t)((crc << 1) ^ 0x31);
		}
		else
		{
			crc <<= 1;
		}
	}

	crc ^= (uint8_t)data; // XOR-in the last input byte
	for (uint8_t i = 0; i < 8; i++)
	{
		if ((crc & 0x80) != 0)
		{
			crc = (uint8_t)((crc << 1) ^ 0x31);
		}
		else
		{
			crc <<= 1;
		}
	}

	return crc; //No output reflection
}


SensorErr SGP30::measure(float temperature, float humidity)
{
	uint32_t absHumid = static_cast<uint32_t>(1000.0f * (216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature))));
	if (absHumid > 256000)
	{
		Publisher::publishEvent('S', "1.6.3", "support.sgp30", "SGP30 humidity compensation failed");
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::measure] - Absolute humidity calculation failed with value: %d", absHumid);
        #endif
        return SensorErr::HUMIDITY_COMPENSTATION_FAILED;
	}

	uint16_t ah_scaled = (uint16_t)(((uint64_t)absHumid * 256 * 16777) >> 24);
	uint8_t args[3];
	args[0] = ah_scaled >> 8;
	args[1] = ah_scaled & 0xFF;
	args[2] = this->crc8(ah_scaled);

	Wire.beginTransmission(SGP30_ADDRS);
	Wire.write(this->set_humidity_compensation, 2);
	Wire.write(args, 3);
	uint8_t res = Wire.endTransmission();
	if (res != 0)
	{
		Publisher::publishEvent('S', "1.6.3", "support.sgp30", "SGP30 humidity compensation failed");
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::measure] - I2C endTransmission returns: %d", res);
        #endif
        return SensorErr::I2C_END_TRANSMISSION_ERR;
	}

	delay(SGP30_DELAY);

	Wire.beginTransmission(SGP30_ADDRS);
	Wire.write(this->measure_air_quality, 2);
	
	res = Wire.endTransmission();
	if (res != 0)
	{
		Publisher::publishEvent('S', "1.6.4", "support.sgp30", "SGP30 measurement failed");
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::measure] - I2C endTransmission returns: %d", res);
        #endif
        return SensorErr::I2C_END_TRANSMISSION_ERR;
	}

	delay(SGP30_DELAY);

	res = Wire.requestFrom(SGP30_ADDRS, (uint8_t)6);
	if (res != 6)
	{
		Publisher::publishEvent('S', "1.6.4", "support.sgp30", "SGP30 measurement failed");
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::measure] - I2C requestFrom returns: %d", res);
        #endif
        return SensorErr::I2C_REQUEST_FROM_ERR;
	}

	uint16_t tmpEco2 = Wire.read() << 8 | Wire.read();
	uint8_t crc = Wire.read();
	if (crc != this->crc8(tmpEco2)){ 
	
		Publisher::publishEvent('S', "1.6.4", "support.sgp30", "SGP30 measurement failed");	
		return SensorErr::CRC_ERR;
	}
	this->eco2 = tmpEco2;

	uint16_t tmpVoc = Wire.read() << 8 | Wire.read();
	crc = Wire.read();
	if (crc != this->crc8(tmpVoc)) {
		
		Publisher::publishEvent('S', "1.6.4", "support.sgp30", "SGP30 measurement failed");
		return SensorErr::CRC_ERR;
	}
	this->voc = tmpVoc;

	return SensorErr::NO_ERR;
}


SensorErr SGP30::selfTest(void)
{
	Wire.beginTransmission(SGP30_ADDRS);  
	Wire.write(this->self_test, 2);
	uint8_t res = Wire.endTransmission();
	if (res != 0)
	{
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::selfTest] - I2C endTransmission returns: %d", res);
        #endif
        return SensorErr::I2C_END_TRANSMISSION_ERR;
	} 

	delay(SGP30_SELF_TEST_DELAY);

	res = Wire.requestFrom(SGP30_ADDRS, (uint8_t)3);
	if (res != 3)
	{
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::selfTest] - I2C requestFrom returns: %d", res);
        #endif
        return SensorErr::I2C_REQUEST_FROM_ERR;
	}

	uint16_t selfTestResult = ((uint16_t)Wire.read()) << 8;		//store MSB in results
	selfTestResult |= Wire.read();								//store LSB in results
	uint8_t crc = Wire.read();									//verify crc
	if (crc != this->crc8(selfTestResult)) return SensorErr::CRC_ERR;

	if (selfTestResult != 0xD400)
	{
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::selfTest] - Self test failed");
        #endif
        return SensorErr::SETUP_ERR;
	} 

	return SensorErr::NO_ERR;
}


SensorErr SGP30::setup(void)
{
	if (this->selfTest() != SensorErr::NO_ERR)
	{
	 
	Publisher::publishEvent('S', "1.6.1", "support.sgp30", "SGP30 startup test failed");
	 return SensorErr::SETUP_ERR;
	}
	Wire.beginTransmission(SGP30_ADDRS);
	Wire.write(init_air_quality, 2);
	uint8_t res = Wire.endTransmission();
	if (res != 0)
	{
		#ifdef LOGGING_SGP30
            Log.error("[SGP30::setup] - I2C endTransmission returns: %d", res);
        #endif
        return SensorErr::I2C_END_TRANSMISSION_ERR;
	}

	delay(SGP30_DELAY);

	return SensorErr::NO_ERR;
}


void SGP30::run(float temperature, float humidity)
{
	if (this->doSetup && this->setupAttempts > 0)
    {
        if (this->setup() == SensorErr::NO_ERR)     // If setup succeeded
        {
            this->setupSucceeded = true;
            this->doSetup = false;
			this->setupTimestamp = millis();

            #ifdef LOGGING_SGP30
                Log.trace("[SGP30::run] - Setup succeeded");
            #endif
        }
        else if (--this->setupAttempts == 0)        // If there are no more setup attempts
        {
			Publisher::publishEvent('S', "1.6.2", "support.sgp30", "SGP30 setup failed");
            this->doSetup = false;

            #ifdef LOGGING_SGP30
                Log.error("[SGP30::run] - Setup failed and there are no more attempts to do a setup");
            #endif
        }
    }

	if (this->setupSucceeded && (millis() - this->setupTimestamp >= 15000) && (millis() - this->lastMeasurement >= SGP30_SAMPLE_TIME || this->lastMeasurement == 0))
	{
        if (this->measure(temperature, humidity) != SensorErr::NO_ERR)
        {
            this->voc = -1;
            this->eco2 = -1;

			this->lastMeasurement = millis();
            
            #ifdef LOGGING_SGP30
                Log.error("[SGP30::run] - Measurement failed");
            #endif
            return;
        }

		#ifdef LOGGING_SGP30
	    	Log.info("[SGP30::run] - sgp30 measured voc: %d", this->getTvoc());
			Log.info("[SGP30::run] - sgp30 measured eco2: %d", this->getEco2());
		#endif

		this->lastMeasurement = millis();
    }
}