#include "Particle.h"
#include "Publisher/Publisher.h"


#define START_BYTE              0xFF
#define NAME_CODE               0x18
// If a packet starts but doesn't complete within this window, discard it.
// Must be >> inter-byte time (~1ms at 9600 baud) but << packet period (1s).
#define ZPH02_PACKET_TIMEOUT_MS 200


enum SensorState
{
    pendingFF,
    pending18,
    reading
};


class ZPH02
{
private:
    SensorState   state;
    uint8_t       index;
    uint8_t       incomingData[9];
    unsigned long packetStartMs;
    int16_t       pm2;
    int16_t       pm10;
    bool          hasValidReading;   // true only after the first checksum-passing packet

    bool dataValidation();

public:
    ZPH02();
    void    run();
    int16_t getPm2()  { return this->pm2;  }
    int16_t getPm10() { return this->pm10; }
    bool    isReady() { return this->hasValidReading; }  // false during warm-up / misconfiguration
};
