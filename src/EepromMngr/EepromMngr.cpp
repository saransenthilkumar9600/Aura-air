#include "EepromMngr.h"


bool EepromMngr::isEepromInit()
{
    // If the EEPROM in index EEPROM_REINIT_FLAG is equal to 255, it means that the EEPROM needs to re-initialized
    // Probably because of SW update that includes changes in the EEPROM structure
    if (EEPROM.read(EEPROM_REINIT_FLAG_ADDRS) == 255)
    {
        #if SAVE_PREV_EEPROM_DATA == true
            this->saveData();
        #endif
        EEPROM.clear();
        EEPROM.write(EEPROM_REINIT_FLAG_ADDRS, true);
    }

    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255) return false;

    return true;
}


#if SAVE_PREV_EEPROM_DATA == true
void EepromMngr::saveData()
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::saveData] - No data to save before initializing the EEPROM");
        #endif
        return;
    }

    EepromMngr::get("scheduler", "size", NULL, &this->tmpContent.schedSize);
    if ((SchedSize)this->tmpContent.schedSize != SchedSize::EMPTY)
    {
        EepromMngr::get("scheduler", "position", NULL, &this->tmpContent.schedPosition);

        this->tmpContent.schedulerDataArr = new TmpSchedulerData[this->tmpContent.schedSize];
        for (int i=0; i<this->tmpContent.schedSize; i++)
        {
            EepromMngr::get("scheduler", "sched" + String(i+1), "whoami", &this->tmpContent.schedulerDataArr[i].whoami);
            EepromMngr::get("scheduler", "sched" + String(i+1), "start-time", (char*)&this->tmpContent.schedulerDataArr[i].startTime);
            EepromMngr::get("scheduler", "sched" + String(i+1), "period", &this->tmpContent.schedulerDataArr[i].period);
        }
    }

    EepromMngr::get("active-mode", "current", NULL, &this->tmpContent.currentMode);
    EepromMngr::get("active-mode", "default", NULL, &this->tmpContent.defaultMode);
    EepromMngr::get("active-mode", "auto-silent-switch", NULL, &this->tmpContent.autoSilentSwitch);
    EepromMngr::get("led-switch", "state", &this->tmpContent.ledSwitch);
    EepromMngr::get("timezone", NULL, NULL, &this->tmpContent.timezone);
    EepromMngr::get("led_scheduler", "enabled", &this->tmpContent.ledScheduler.enabled);
    EepromMngr::get("led_scheduler", "start", NULL, this->tmpContent.ledScheduler.startTime);
    EepromMngr::get("led_scheduler", "end", NULL, this->tmpContent.ledScheduler.endTime);
    EepromMngr::get("led_scheduler", "state", &this->tmpContent.ledScheduler.state);
    // EepromMngr::get("fan-switch", "state", &this->tmpContent.fanSwitch);
}
#endif


void EepromMngr::initEeprom()
{
    EepromMngr instance;
    if (instance.isEepromInit())
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::initEeprom] - EEPROM already initialized");
        #endif
        return;
    }
    #ifdef LOGGING_EEPROM
        Log.trace("[EepromMngr::initEeprom] - Initializing the EEPROM");
    #endif

    EepromContent content;
    DynamicJsonDocument jDoc(JSON_DOC_SIZE);

    #if SAVE_PREV_EEPROM_DATA == true
        if ((SchedSize)instance.tmpContent.schedSize != SchedSize::EMPTY)
        {
            jDoc["scheduler"]["size"] = instance.tmpContent.schedSize;
            jDoc["scheduler"]["position"] = instance.tmpContent.schedPosition;

            for (uint8_t i=0; i<instance.tmpContent.schedSize; i++)
            {
                char schedNum[6] = "sched";
                strcat(schedNum, String(i+1));
                jDoc["scheduler"][schedNum]["whoami"] = instance.tmpContent.schedulerDataArr[i].whoami;
                jDoc["scheduler"][schedNum]["start-time"] = instance.tmpContent.schedulerDataArr[i].startTime;
                jDoc["scheduler"][schedNum]["period"] = instance.tmpContent.schedulerDataArr[i].period;
                if (i+1 == 1)
                    jDoc["scheduler"][schedNum]["tmp-period"] = 0.0;
            }

            for (uint8_t i=6-(6-instance.tmpContent.schedSize); i<6; i++)
            {
                char schedNum[6] = "sched";
                strcat(schedNum, String(i+1));
                jDoc["scheduler"][schedNum]["whoami"] = (int8_t)SysMode::NOT_INIT;
                jDoc["scheduler"][schedNum]["start-time"] = NULL;
                jDoc["scheduler"][schedNum]["period"] = 0.0;
            }

            delete [] instance.tmpContent.schedulerDataArr;
        }
        else
        {
            jDoc["scheduler"]["size"] = (uint8_t)SchedSize::EMPTY;
            jDoc["scheduler"]["position"] = (uint8_t)SchedPosition::FIRST;

            for (uint8_t i=0; i<6; i++)
            {
                char schedNum[6] = "sched";
                strcat(schedNum, String(i+1));
                jDoc["scheduler"][schedNum]["whoami"] = (int8_t)SysMode::NOT_INIT;
                jDoc["scheduler"][schedNum]["start-time"] = NULL;
                jDoc["scheduler"][schedNum]["period"] = 0.0;
                if (i+1 == 1)
                    jDoc["scheduler"][schedNum]["tmp-period"] = 0.0;
            }
        }

        jDoc["active-mode"]["current"] = instance.tmpContent.currentMode;
        jDoc["active-mode"]["default"] = instance.tmpContent.defaultMode;
        jDoc["active-mode"]["auto-silent-switch"] = instance.tmpContent.autoSilentSwitch;

        jDoc["panic-reset"]["panic-counter"] = (uint8_t)0;
        jDoc["panic-reset"]["last-panic-event"] = (uint8_t)0;

        jDoc["user-reset"]["reset-counter"] = (uint8_t)0;
        jDoc["user-reset"]["last-reset-event"] = (uint8_t)0;

        jDoc["uvc-switch"] = (uint8_t)SysEvent::UVC_AUTO;

        jDoc["sterio-switch"] = (uint8_t)SysEvent::STERIO_AUTO;

        jDoc["led-switch"]["state"] = instance.tmpContent.ledSwitch;

        jDoc["led_scheduler"]["enabled"] = instance.tmpContent.ledScheduler.enabled;
        jDoc["led_scheduler"]["start"] = instance.tmpContent.ledScheduler.startTime;
        jDoc["led_scheduler"]["end"] = instance.tmpContent.ledScheduler.endTime;
        jDoc["led_scheduler"]["state"] = instance.tmpContent.ledScheduler.state;

        jDoc["timezone"] = instance.tmpContent.timezone;


        // jDoc["fan-switch"]["state"] = instance.tmpContent.fanSwitch;
    #else
        jDoc["scheduler"]["size"] = (uint8_t)SchedSize::EMPTY;
        jDoc["scheduler"]["position"] = (uint8_t)SchedPosition::FIRST;

        for (uint8_t i=0; i<6; i++)
        {
            char schedNum[6] = "sched";
            strcat(schedNum, String(i+1));
            jDoc["scheduler"][schedNum]["whoami"] = (int8_t)SysMode::NOT_INIT;
            jDoc["scheduler"][schedNum]["start-time"] = NULL;
            jDoc["scheduler"][schedNum]["period"] = 0.0;
            if (i+1 == 1)
                jDoc["scheduler"][schedNum]["tmp-period"] = 0.0;
        }

        jDoc["active-mode"]["current"] = (int8_t)SysMode::LOW_M;
        jDoc["active-mode"]["default"] = (int8_t)SysMode::LOW_M;

        jDoc["panic-reset"]["panic-counter"] = (uint8_t)0;
        jDoc["panic-reset"]["last-panic-event"] = (uint8_t)0;

        jDoc["user-reset"]["reset-counter"] = (uint8_t)0;
        jDoc["user-reset"]["last-reset-event"] = (uint8_t)0;

        jDoc["uvc-switch"] = (uint8_t)SysEvent::UVC_AUTO;

        jDoc["sterio-switch"] = (uint8_t)SysEvent::STERIO_AUTO;

        jDoc["led-switch"]["state"] = true;

        jDoc["led_scheduler"]["enabled"] = false;
        jDoc["led_scheduler"]["start"] = "00:00";
        jDoc["led_scheduler"]["end"] = "00:00";
        jDoc["led_scheduler"]["state"] = true;

        jDoc["timezone"] = (uint8_t)0;
    #endif

    serializeJson(jDoc, content.jDoc);

    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, content);     // Save the Eeprom content object in the EEPROM
    EEPROM.write(EEPROM_IS_DEFINED_ADDRS, true);    // Mark the EEPROM as initialized
}


String EepromMngr::printEeprom()
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::printEeprom] - EEPROM marked as not initialized");
        #endif

        Particle.publish("internal_support", "EEPROM marked as not initialized", PRIVATE);
        
        return "NULL";
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content);
    Particle.publish("internal_support", String(content.jDoc), PRIVATE);

    return String(content.jDoc);
}


void EepromMngr::clearEeprom()
{
    EEPROM.clear();
    Publisher::publishEvent('S', "1.2.1", "support.eeprom", "The EEPROM cleared successfully");
}


int EepromMngr::getMemUsage()
{
    uint16_t usedSpace = 0;
    for (int16_t i=0; i<2048; i++)
    {
        uint8_t val = EEPROM.read(i);
        if (val != 255) usedSpace++;
    }
    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::getMemUsage] - %d bytes in use", usedSpace);
    #endif
    return usedSpace;
}


void EepromMngr::set(const char *parentKey, const char *childKey, bool value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey].set(value);
    }

    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
    
}


void EepromMngr::get(const char *parentKey, const char *childKey, bool *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::get] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else
    {
        *target = jDoc[parentKey][childKey];
    }
}


void EepromMngr::set(const char *parentKey, const char *childKey, const char *secChildKey, bool value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else if (secChildKey == NULL)
    {
        jDoc[parentKey][childKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey][secChildKey].set(value);
    }

    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
}


void EepromMngr::get(const char *parentKey, const char *childKey, const char *secChildKey, bool *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return; 

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else if (secChildKey == NULL)
    {
        *target = jDoc[parentKey][childKey];
    }
    else
    {
        *target = jDoc[parentKey][childKey][secChildKey];
    }
}

void EepromMngr::set(const char *parentKey, const char *childKey, const char *secChildKey, uint8_t value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else if (secChildKey == NULL)
    {
        jDoc[parentKey][childKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey][secChildKey].set(value);
    }

    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
}


void EepromMngr::get(const char *parentKey, const char *childKey, const char *secChildKey, uint8_t *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return; 

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else if (secChildKey == NULL)
    {
        *target = jDoc[parentKey][childKey];
    }
    else
    {
        *target = jDoc[parentKey][childKey][secChildKey];
    }
}


void EepromMngr::set(const char *parentKey, const char *childKey, const char *secChildKey, int8_t value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else if (secChildKey == NULL)
    {
        jDoc[parentKey][childKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey][secChildKey].set(value);
    }

    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
}


void EepromMngr::get(const char *parentKey, const char *childKey, const char *secChildKey, int8_t *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::get] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else if (secChildKey == NULL)
    {
        *target = jDoc[parentKey][childKey];
    }
    else
    {
        *target = jDoc[parentKey][childKey][secChildKey];
    }
}


void EepromMngr::set(const char *parentKey, const char *childKey, const char *secChildKey, int16_t value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else if (secChildKey == NULL)
    {
        jDoc[parentKey][childKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey][secChildKey].set(value);
    }

    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
}


void EepromMngr::get(const char *parentKey, const char *childKey, const char *secChildKey, int16_t *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::get] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else if (secChildKey == NULL)
    {
        *target = jDoc[parentKey][childKey];
    }
    else
    {
        *target = jDoc[parentKey][childKey][secChildKey];
    }
}


void EepromMngr::set(const char *parentKey, const char *childKey, const char *secChildKey, float value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return; 

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else if (secChildKey == NULL)
    {
        jDoc[parentKey][childKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey][secChildKey].set(value);
    }

    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
}


void EepromMngr::get(const char *parentKey, const char *childKey, const char *secChildKey, float *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return; 

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else if (secChildKey == NULL)
    {
        *target = jDoc[parentKey][childKey];
    }
    else
    {
        *target = jDoc[parentKey][childKey][secChildKey];
    }
}


void EepromMngr::set(const char *parentKey, const char *childKey, const char *secChildKey, const char *value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else if (secChildKey == NULL)
    {
        jDoc[parentKey][childKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey][secChildKey].set(value);
    }
    
    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
}


void EepromMngr::get(const char *parentKey, const char *childKey, const char *secChildKey, char *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else if (secChildKey == NULL)
    {
        const char *tmp = jDoc[parentKey][childKey];
        if (tmp != nullptr)
            strncpy(target, tmp, 5);
        target[5] = '\0';
    }
    else
    {
        const char *tmp = jDoc[parentKey][childKey][secChildKey];
        if (tmp != nullptr)
            strncpy(target, tmp, 5);
        target[5] = '\0';
    }
}


void EepromMngr::set(const char *parentKey, const char *childKey, const char *secChildKey, unsigned long value)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::set] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    #ifdef LOGGING_EEPROM
        Log.info("[EepromMngr::set] - Dynamic json document mem usage is: %lu", (unsigned long)jDoc.memoryUsage());
    #endif

    if (childKey == NULL)
    {
        jDoc[parentKey].set(value);
    }
    else if (secChildKey == NULL)
    {
        jDoc[parentKey][childKey].set(value);
    }
    else
    {
        jDoc[parentKey][childKey][secChildKey].set(value);
    }

    EepromContent newContent;
    serializeJson(jDoc, newContent.jDoc);
    EEPROM.put(EEPROM_DATA_OBJ_ADDRS, newContent);
}


void EepromMngr::get(const char *parentKey, const char *childKey, const char *secChildKey, unsigned long *target)
{
    if (EEPROM.read(EEPROM_IS_DEFINED_ADDRS) == 255)
    {
        #ifdef LOGGING_EEPROM
            Log.info("[EepromMngr::get] - EEPROM marked as not initialized");
        #endif
        return;
    }

    EepromContent content;
    EEPROM.get(EEPROM_DATA_OBJ_ADDRS, content); // Restore the EEPROM content

    DynamicJsonDocument jDoc(JSON_DOC_SIZE);
    DeserializationError err = deserializeJson(jDoc, content.jDoc);
    if (err) return;

    if (childKey == NULL)
    {
        *target = jDoc[parentKey];
    }
    else if (secChildKey == NULL)
    {
        *target = jDoc[parentKey][childKey];
    }
    else
    {
        *target = jDoc[parentKey][childKey][secChildKey];
    }
}

/*jDoc["rs485"]["id"] = (int16_t)-1;

    jDoc["scheduler"]["size"] = (uint8_t)SchedSize::EMPTY;
    jDoc["scheduler"]["position"] = (uint8_t)SchedPosition::FIRST;
    jDoc["scheduler"]["to-increment"] = false;

    jDoc["scheduler"]["sched1"]["whoami"] = (int8_t)SysMode::NOT_INIT;
    jDoc["scheduler"]["sched1"]["start-time"] = NULL;
    jDoc["scheduler"]["sched1"]["period"] = 0.0;
    jDoc["scheduler"]["sched1"]["tmp-period"] = 0.0;

    jDoc["scheduler"]["sched2"]["whoami"] = (int8_t)SysMode::NOT_INIT;
    jDoc["scheduler"]["sched2"]["start-time"] = NULL;
    jDoc["scheduler"]["sched2"]["period"] = 0.0;

    jDoc["scheduler"]["sched3"]["whoami"] = (int8_t)SysMode::NOT_INIT;
    jDoc["scheduler"]["sched3"]["start-time"] = NULL;
    jDoc["scheduler"]["sched3"]["period"] = 0.0;

    jDoc["scheduler"]["sched4"]["whoami"] = (int8_t)SysMode::NOT_INIT;
    jDoc["scheduler"]["sched4"]["start-time"] = NULL;
    jDoc["scheduler"]["sched4"]["period"] = 0.0;

    jDoc["active-mode"]["current"] = (int8_t)SysMode::AUTO_M;
    jDoc["active-mode"]["default"] = (int8_t)SysMode::AUTO_M;

    jDoc["panic-reset"]["panic-counter"] = 0;
    jDoc["panic-reset"]["last-panic-event"] = NULL;

    jDoc["user-reset"]["reset-counter"] = 0;
    jDoc["user-reset"]["last-reset-event"] = NULL;

    jDoc["led-switch"]["state"] = true;

    jDoc["timezone"] = NULL;*/