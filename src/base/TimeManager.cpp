#include "TimeManager.h"

#ifdef ESP8266
#include <user_interface.h>
#else
#include <esp_sntp.h>
#endif

uint32_t TimeManager::_ntpEpoch = 0;
uint32_t TimeManager::_ntpMillis = 0;

void TimeManager::init(const char *ntpServer)
{
    configTime(0, 0, ntpServer);
    settimeofday_cb([]()
                    {
        time_t t = time(nullptr);
        if (t > 1000000000UL)
        {
            _ntpEpoch = (uint32_t)t;
            _ntpMillis = millis();
        } });
}

uint32_t TimeManager::getCurrentEpoch()
{
    if (_ntpEpoch == 0)
        return 0;

    uint32_t currentMillis = millis();
    uint32_t elapsedSeconds = (currentMillis - _ntpMillis) / 1000;
    return _ntpEpoch + elapsedSeconds;
}
