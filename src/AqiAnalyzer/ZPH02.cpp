#include "ZPH02.h"


ZPH02::ZPH02()
{
    this->state            = pendingFF;
    this->index            = 0;
    this->packetStartMs    = 0;
    this->pm2              = 0;
    this->pm10             = 0;
    this->hasValidReading  = false;      // stays false until first checksum-passing packet
    this->incomingData[9]  = {0};
}


void ZPH02::run()
{
    // Bug 1+2 fix: stale-packet timeout — if we started a packet but it
    // didn't complete within 200ms, reset state so we don't stay stuck
    // in pending18/reading forever with a corrupted index.
    if (this->state != pendingFF &&
        (millis() - this->packetStartMs > ZPH02_PACKET_TIMEOUT_MS))
    {
        this->state = pendingFF;
        this->index = 0;
    }

    // Bug 1 fix: drain whatever bytes are already in Serial1's RX buffer
    // and return immediately. No blocking, no poll counter.
    // The persistent state/index pick up where we left off next call.
    while (Serial1.available())
    {
        uint8_t value = Serial1.read();

        if (this->state == pendingFF)
        {
            if (value == START_BYTE)
            {
                this->index = 0;
                this->incomingData[this->index++] = value;
                this->packetStartMs = millis();
                this->state = pending18;
            }
        }
        else if (this->state == pending18)
        {
            if (value == NAME_CODE)
            {
                this->incomingData[this->index++] = value;
                this->state = reading;
            }
            else
            {
                this->state = pendingFF;
                this->index = 0;
            }
        }
        else if (this->state == reading)
        {
            this->incomingData[this->index++] = value;

            if (this->index > 8)
            {
                if (this->dataValidation())
                {
                    this->pm2  = round((this->incomingData[3] + 0.01 * this->incomingData[4])/0.1);
                    this->pm10 = round((this->pm2 + 6.0767) / 0.779);
                    this->hasValidReading = true;   // latch: sensor confirmed alive and readable
                }

                // Bug 2 fix: always reset state after a complete packet
                // (whether checksum passed or failed) so the next call starts clean.
                this->state = pendingFF;
                this->index = 0;
            }
        }
    }
}


bool ZPH02::dataValidation()
{
    uint8_t tempq = 0;

    // sum parts 1 to 7
    for (uint8_t j = 1; j < 8; j++)
    {
        tempq += this->incomingData[j];
    }

    tempq = (~tempq) + 1;

    if (tempq == this->incomingData[8])
    {
        return true;
    }

    return false;
}
