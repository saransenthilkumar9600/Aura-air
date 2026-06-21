#include "Publisher.h"


retained uint8_t publishQueueRetainedBuffer[2048];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));


void Publisher::setup()
{
    publishQueue.setup();
    //publishQueue.publish(NORMAL_EVENT,"publishQueue setup", PRIVATE, WITH_ACK);
}

void Publisher::publishEvent(char event_t, String code, String name, String type, char *message)
{
    if(event_t == 'N') // N is a sign to normal event
    {
        //if(message!="")
            publishQueue.publish(NORMAL_EVENT, String(message), PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
    else if(event_t == 'S') // 'S' is a sign to support event
    {
        //if(message!="")
            publishQueue.publish(SUPPORT_EVENT, "{\"code\":\"" + code + "\", \"name\":\"" + name + "\", \"type\":\"" + type + "\", \"message\":\"" + String(message) + "\"}", PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
}


void Publisher::publishEvent(char event_t, String code, String name, String message)
{
    if (event_t == 'N') // N is a sign to normal event
    {
        //if(message!="")
            publishQueue.publish(NORMAL_EVENT, message, PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
    else if (event_t == 'S') // 'S' is a sign to support event
    {
        //if(message!="")
            publishQueue.publish(SUPPORT_EVENT, "{\"code\":\"" + code + "\", \"name\":\"" + name + "\", \"message\":\"" + message + "\"}", PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
}


void Publisher::publishEvent(char event_t, String code, String name, String message, bool jsonMsgFlag)
{
    if (event_t == 'N') // N is a sign to normal event
    {
        //if(message!="")
            publishQueue.publish(NORMAL_EVENT, message, PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
    else if (event_t == 'S') // 'S' is a sign to support event
    {
        char buffer[750];
        JSONBufferWriter jwriter(buffer, sizeof(buffer)-1);
        jwriter.beginObject();
            jwriter.name("code").value(code);
            jwriter.name("name").value(name);
            jwriter.name("message").value(message);
        jwriter.endObject();
        
        //if(message!="")
            publishQueue.publish(SUPPORT_EVENT, buffer, PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
}


void Publisher::publishEvent(char event_t, String code, String name, String type, unsigned int message)
{
    if (event_t == 'N') // N is a sign to normal event
    {
        if(String(message)!="")
            publishQueue.publish(NORMAL_EVENT, String(message), PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
    else if (event_t == 'S') // 'S' is a sign to support event
    {
        if(String(message)!="")
            publishQueue.publish(SUPPORT_EVENT, "{\"code\":\"" + code + "\", \"name\":\"" + name + "\", \"type\":\"" + type + "\", \"message\":\"" + String(message) + "\"}", PRIVATE, WITH_ACK);
        //delay(PUB_DELAY);
    }
}

bool Publisher::clearMemory()
{
    return publishQueue.clearEvents();
}