#ifndef PUBLISHER_H
#define PUBLISHER_H



#include "Particle.h"

// This must be included before PublishQueueAsyncRK.h to add in SPIFFS support
#include "../../lib/PublishQueueAsyncRK/src/PublishQueueAsyncRK.h"


#define PUB_DELAY         0
#define NORMAL_EVENT      "auradb"
#define SUPPORT_EVENT     "support"
#define TEST_EVENT        "testdb"



class Publisher
{
public:
    static void setup();
    static bool clearMemory();
    static void publishEvent(char, String, String, String, char *);
    static void publishEvent(char, String, String, String);
    static void publishEvent(char, String, String, String, bool);
    static void publishEvent(char, String, String, String, unsigned int);
};

#endif
