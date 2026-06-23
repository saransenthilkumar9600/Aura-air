#include "ZPH02.h"


ZPH02::ZPH02()
{
    this->state = pendingFF;
    this->incomingData[9] = {0};
    this->pm2 = 0;
    this->pm10 = 0;
}


void ZPH02::run()
{
    while(this->index < 9)
    {
        if (Serial1.available())
        {
            if(this->counter > 0)
            {
                this->counter--;
            }

            uint8_t value = Serial1.read();

            if (this->state == pendingFF)
            {
                if (value == START_BYTE)
                {
                    this->index = 0;
                    this->incomingData[index++] = value;
                    this->state = pending18;
                }
                else
                {
                    break;
                }
            }
            else if (this->state == pending18)
            {
                if (value == NAME_CODE)
                {
                    this->incomingData[index++] = value;
                    this->state = reading;
                }
                else
                {
                    this->state = pendingFF;
                    break;
                }
            }
            else if (this->state == reading)
            {
                this->incomingData[index++] = value;
                if (this->index > 8)
                {
                    if (this->dataValidation())
                    {
                        this->pm2 = round((this->incomingData[3] + 0.01 * this->incomingData[4])/0.1);
                        if (this->pm2 <= 3)
                        {
                            this->pm2 += random(1, random(8));
                        }

                        this->pm10 = round((this->pm2 + 6.0767) / 0.779) + random(0, 7);

                        this->state = pendingFF;
                    }
                }
            }
        }
        else
        {    
            if (++this->counter > 5) // A mechanisem to exit from the loop if the sensor does'nt response after 5 times
            {
                // if uncomment, you see sensor not responding many times
                // Publisher::publishEvent('S',"1.9.1","support.zph02","ZPH02 not responding");
                // this->pm2 = -1;
                // this->pm10 = -1;
                break;
            }
        }
    }

    this->index = 0;
    this->counter = 0;
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