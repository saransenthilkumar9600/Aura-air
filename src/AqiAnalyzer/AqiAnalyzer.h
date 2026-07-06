#include <functional>
#include <algorithm>
#include "Components/Fan.h"
#include "Publisher/Publisher.h"
#include "enums.hpp"
#include "HDC1080.h"
#include "ME2_CO.h"
#include "ZPH02.h"
#include "SGP30.h"


using namespace std;


// #define LOGGING_AQILYZER
#define MOVING_AVG_SIZE       5
#define ECO2_ARRAY_SIZE       50
#define PUBLISH_DATA_INTERVAL 5000


typedef std::function<void(SysEvent)> callbackFunc;


class AqiAnalyzer
{
private:
    static AqiAnalyzer *instance;
    Fan &fan;
    HDC1080 hdc1080;
    SGP30 sgp30;
    ZPH02 zph02;
    ME2_CO co;
    char dp[2];
    float aqiMovingAvg[MOVING_AVG_SIZE];
    uint8_t currentAQI;
    uint8_t aqiMovingMvgCurrIdx;
    int16_t Eco2Arr[ECO2_ARRAY_SIZE];
    uint8_t count = 0;
    unsigned long lastSensorsDataPublishedTimestamp;
    SysEvent lastFiredAqiEvent;
    callbackFunc execComponentsChainCallback;

    AqiAnalyzer();
    void calcAqiMovingAvg(uint16_t);
    void analyze();
    uint16_t findMaxIndex(uint16_t[]);
    uint16_t getCo2AqiClass(int16_t);
    uint16_t getTvocAqiClass(int16_t);
    uint16_t getPm2AqiClass(int16_t);
    uint16_t getPm10AqiClass(int16_t);
    uint16_t getCoAqiClass(float);

public:
    static AqiAnalyzer& getInstance();
    void run();
    void setup(callbackFunc);
};