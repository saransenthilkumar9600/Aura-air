#include "AqiAnalyzer.h"


AqiAnalyzer *AqiAnalyzer::instance = nullptr;


AqiAnalyzer::AqiAnalyzer() : fan(Fan::getInstance())
{
    this->aqiMovingMvgCurrIdx = 0;
    for (uint8_t i = 0; i < MOVING_AVG_SIZE; i++){
        this->aqiMovingAvg[i] = 0.0;
    }

    for (uint8_t j = 0; j < ECO2_ARRAY_SIZE; j++){
        this->Eco2Arr[j] = 0;
    }
     
}


AqiAnalyzer& AqiAnalyzer::getInstance()
{
    if (instance == nullptr)
        instance = new AqiAnalyzer();

    return *instance;
}


void AqiAnalyzer::setup(callbackFunc execComponentsChainCallback)
{
    this->execComponentsChainCallback = execComponentsChainCallback;
}

uint16_t AqiAnalyzer::findMaxIndex(uint16_t *arrAqi)
{
    uint16_t indexOfMax = 0;

    for (uint16_t i=1; i<5; i++)
    {
        if(arrAqi[indexOfMax] < arrAqi[i])
        {
            indexOfMax = i;
        }
    }

    return indexOfMax;
}

void AqiAnalyzer::run()
{
    this->hdc1080.run();
    this->sgp30.run(this->hdc1080.getTemperature(), this->hdc1080.getHumidity());
    this->zph02.run();
    this->co.run();

    uint16_t aqiArray[5];
    aqiArray[0] = getCo2AqiClass(this->sgp30.getEco2());
    aqiArray[1] = getTvocAqiClass(this->sgp30.getTvoc());
    // Guard: only include PM readings in AQI once ZPH02 has confirmed a valid packet.
    // Before that, pm2 = 0 which is not a real sensor value (sensor minimum is 5 µg/m³).
    aqiArray[2] = this->zph02.isReady() ? getPm2AqiClass(this->zph02.getPm2())  : 0;
    aqiArray[3] = this->zph02.isReady() ? getPm10AqiClass(this->zph02.getPm10()) : 0;
    aqiArray[4] = getCoAqiClass(this->co.getCo());

    // uint8_t idxOfMax = distance(aqiArray, max_element(aqiArray, aqiArray+5));
    uint16_t idxOfMax = findMaxIndex(aqiArray);

    #ifdef LOGGING_AQILYZER
        Log.info("[AqiAnalyzer::run] - Max element index is %d", (uint8_t)idxOfMax);
    #endif

    if (this->dp != NULL)
    {
        this->dp[0] = 'v';
        if (idxOfMax == 0) this->dp[1] = '3';
        else if (idxOfMax == 1) this->dp[1] = '4';
        else if (idxOfMax == 2) this->dp[1] = '5';
        else if (idxOfMax == 3) this->dp[1] = '6';
        else if (idxOfMax == 4) this->dp[1] = '9';
    }

    this->calcAqiMovingAvg(aqiArray[idxOfMax]);
    // this->currentAQI = aqiArray[idxOfMax];
    this->analyze();

    if (millis() - this->lastSensorsDataPublishedTimestamp > PUBLISH_DATA_INTERVAL)
    {
        this->lastSensorsDataPublishedTimestamp = millis();
        
        this->Eco2Arr[this->count] = this->sgp30.getEco2();
        this->count++;

        if(this->count >= ECO2_ARRAY_SIZE)
            this->count = 0;
         
        if (Particle.connected()) 
        {
        #ifdef LOGGING_AQILYZER
            Log.trace("[AqiAnalyzer::run] - Publishing sensors data to cloud");
        #endif
        
        char tmpSensorsData[550];
            // v1:temp, v2:humid, v3:Co2, v4:tvoc, v5:pm2, v6:pm10, v7:dominent pollutant , v8:aqi, v9:co, v10:current fan speed
            snprintf(tmpSensorsData, sizeof(tmpSensorsData), "{\"v1\":%.1f, \"v2\":%.1f, \"v3\":%d, \"v4\":%d, \"v5\":%d, \"v6\":%d, \"v7\":\"%c%c\", \"v8\":%.1f, \"v9\":%.1f, \"v10\":%d, \"v11\": [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d] }",
            this->hdc1080.getTemperature(), this->hdc1080.getHumidity(),
            this->sgp30.getEco2(), this->sgp30.getTvoc(),
            // v5/v6: publish -1 when ZPH02 has not yet confirmed a valid packet
            // (sensor needs up to 60s warm-up per datasheet; pm2=0 before that is not a real reading).
            // Dashboard must treat -1 as "sensor initializing" and skip plotting that point.
            this->zph02.isReady() ? (int16_t)this->zph02.getPm2()  : (int16_t)-1,
            this->zph02.isReady() ? (int16_t)this->zph02.getPm10() : (int16_t)-1,
            this->dp[0], this->dp[1],
            this->aqiMovingAvg[aqiMovingMvgCurrIdx] / 10.0,
            // this->currentAQI,
            this->co.getCo(),
            (uint16_t)this->fan.getFanSpeed(),
            this->Eco2Arr[0], this->Eco2Arr[1], this->Eco2Arr[2], this->Eco2Arr[3], this->Eco2Arr[4], this->Eco2Arr[5], this->Eco2Arr[6], this->Eco2Arr[7], this->Eco2Arr[8], this->Eco2Arr[9],
            this->Eco2Arr[10], this->Eco2Arr[11], this->Eco2Arr[12], this->Eco2Arr[13], this->Eco2Arr[14], this->Eco2Arr[15], this->Eco2Arr[16], this->Eco2Arr[17], this->Eco2Arr[18], this->Eco2Arr[19],
            this->Eco2Arr[20], this->Eco2Arr[21], this->Eco2Arr[22], this->Eco2Arr[23], this->Eco2Arr[24], this->Eco2Arr[25], this->Eco2Arr[26], this->Eco2Arr[27], this->Eco2Arr[28], this->Eco2Arr[29],
            this->Eco2Arr[30], this->Eco2Arr[31], this->Eco2Arr[32], this->Eco2Arr[33], this->Eco2Arr[34], this->Eco2Arr[35], this->Eco2Arr[36], this->Eco2Arr[37], this->Eco2Arr[38], this->Eco2Arr[39],
            this->Eco2Arr[40], this->Eco2Arr[41], this->Eco2Arr[42], this->Eco2Arr[43], this->Eco2Arr[44], this->Eco2Arr[45], this->Eco2Arr[46], this->Eco2Arr[47], this->Eco2Arr[48], this->Eco2Arr[49]);
            
        
        

        Publisher::publishEvent('N', "", "", "", tmpSensorsData);
        #ifdef LOGGING_AQILYZER
            Log.trace("[AqiAnalyzer::run] - %s", tmpSensorsData);
        #endif

        }
    }
}


void AqiAnalyzer::calcAqiMovingAvg(uint16_t currAqi)
{
    #ifdef LOGGING_AQILYZER
        Log.info("[AqiAnalyzer::calcAqiMovingAvg] - Current AQI is %d", currAqi);
    #endif
    this->aqiMovingMvgCurrIdx++;

    if (this->aqiMovingMvgCurrIdx == MOVING_AVG_SIZE) 
        this->aqiMovingMvgCurrIdx = 0;

    if (this->aqiMovingMvgCurrIdx == 0)
        this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] = (currAqi + this->aqiMovingAvg[MOVING_AVG_SIZE-1]) / 2;
    else
        this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] = (currAqi + this->aqiMovingAvg[this->aqiMovingMvgCurrIdx-1]) / 2;

    if (this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] < 10.0)
        this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] = 10.0; 

    #ifdef LOGGING_AQILYZER
        Log.info("[AqiAnalyzer::calcAqiMovingAvg] - Calculated AQI is %.1f", this->aqiMovingAvg[this->aqiMovingMvgCurrIdx]);
    #endif
}


void AqiAnalyzer::analyze()
{
    if (this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] < 50.0)
        this->execComponentsChainCallback(SysEvent::AQI_LESS_50);
    else if (this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] < 100.0)
        this->execComponentsChainCallback(SysEvent::AQI_LESS_100);
    else if (this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] < 200.0)
        this->execComponentsChainCallback(SysEvent::AQI_LESS_200);
    else if (this->aqiMovingAvg[this->aqiMovingMvgCurrIdx] <= 500.0)
        this->execComponentsChainCallback(SysEvent::AQI_LESS_500);
}



uint16_t AqiAnalyzer::getCo2AqiClass(int16_t Eco2)
{

    if (Eco2 < 350)
        return 0;
    // GREAT
    if (Eco2 >= 350 && Eco2 < 884.98) 
        return floor((Eco2 - 350) / 10.52);
    // GOOD
    else if (Eco2 < 1333.5) 
        return floor(((Eco2 - 884.98) / 8.98) + 51);
    // MODERATE
    else if (Eco2 < 1765.76) 
        return floor(((Eco2 - 1333.5) / 8.5) + 101);
    // LOW
    else if (Eco2 < 2551.08) 
        return floor(((Eco2 - 1765.76) / 15.76) + 151);
    // POOR
    else if (Eco2 < 3851.77) 
        return floor(((Eco2 - 2551.08) / 13.08) + 201);
    // BAD
    else if (Eco2 < 5000) 
        return floor(((Eco2 - 3851.77) / 5.77) + 301);
    // OUT OF BOUNDS
    else if (Eco2 >= 5000)
        return 500;
    else return 0;

    
}


uint16_t AqiAnalyzer::getTvocAqiClass(int16_t tvoc)
{
    // GREAT
    if (tvoc >= 0 && tvoc < 300) return floor(tvoc / 6);
    // GOOD
    else if (tvoc < 500) return floor((0.25 * (tvoc - 300) + 50));
    // MODERATE
    else if (tvoc < 1000) return floor((0.1 * (tvoc - 500) + 100));
    // LOW
    else if (tvoc < 1230) return floor(((tvoc - 1000) / 40.0 + 150));
    // POOR
    else if (tvoc < 4000) {
        return floor((0.1 * (tvoc - 3000) + 200));
    }
    // BAD
    else if (tvoc < 5000) {
        return floor((0.2 * (tvoc - 4000) + 300));
    }
    // OUT OF BOUNDS
    else if (tvoc >= 5000) {
        return 500;
    }
    
    else return 0;
}


uint16_t AqiAnalyzer::getPm2AqiClass(int16_t pm2)
{
    // GREAT
    if (pm2 >= 0 && pm2 < 12.1) 
        return floor(pm2 / 0.24);
    // GOOD
    else if (pm2 < 35.13) 
        return floor(((pm2 - 12.1) / 0.47) + 51);
    // MODERATE
    else if (pm2 < 55.5) 
        return floor(((pm2 - 35.5) / 0.4) + 101);
    // LOW
    else if (pm2 < 150.5) 
        return floor(((pm2 - 55.5) / 1.9) + 151);
    // POOR
    else if (pm2 < 250.5) 
        return floor(((pm2 - 150.5) / 1) + 201);
    // BAD
    else if (pm2 < 500.4) 
        return floor(((pm2 - 250.5) / 1.25) + 301);
    // OUT OF BOUNDS
    else if(pm2 >= 500.4)
        return 500;
    else return 0;
}


uint16_t AqiAnalyzer::getPm10AqiClass(int16_t pm10)
{
    // GREAT
    if (pm10 >= 0 && pm10 < 55) 
        return floor(pm10 / 1.08);
    // GOOD
    else if (pm10 < 155) 
        return floor(((pm10 - 55) / 2) + 51);
    // MODERATE
    else if (pm10 < 255) 
        return floor(((pm10 - 155) / 2) + 101);
    // LOW
    else if (pm10 < 355) 
        return floor(((pm10 - 255) / 2) + 151);
    // POOR
    else if (pm10 < 425) 
        return floor(((pm10 - 355) / 0.7) + 201);
    // BAD
    else if (pm10 < 604) 
        return floor(((pm10 - 425) / 0.9) + 301);
    // OUT OF BOUNDS
    else if(pm10 >= 604)
        return 500;
    else return 0;
}


uint16_t AqiAnalyzer::getCoAqiClass(float co)
{
    // GREAT
    if (co >= 0 && co < 4.5) 
        return floor(co / 0.09);
    // GOOD
    else if (co < 9.5) 
        return floor(((co - 4.5) / 0.1) + 51);
    // MODERATE
    else if (co < 12.5) 
        return floor(((co - 9.5) / 0.06) + 101);
    // LOW
    else if (co < 15.5) 
        return floor(((co - 12.5) / 0.06) + 151);
    // POOR
    else if (co < 30.5) 
        return floor(((co - 15.5) / 0.15) + 201);
    // BAD
    else if (co < 50.4) 
        return floor(((co - 30.5) / 0.1) + 301);
    // OUT OF BOUNDS
    else if(co >= 50.4)
        return 500;
    else return 0;
}