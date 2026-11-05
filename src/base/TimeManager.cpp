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

void TimeManager::setTimezone(const char *tzString)
{
    if (tzString && tzString[0])
    {
        setenv("TZ", tzString, 1);
        tzset();
    }
}

int TimeManager::getUTCTime(char *buf, size_t bufSize)
{
    if (_ntpEpoch == 0)
        return 0;

    uint32_t currentMillis = millis();
    uint32_t elapsedSeconds = (currentMillis - _ntpMillis) / 1000;
    uint32_t epoch = _ntpEpoch + elapsedSeconds;

    time_t t = (time_t)epoch;
    struct tm tmInfo;
    gmtime_r(&t, &tmInfo);
    return snprintf_P(buf, bufSize, PSTR("%04d-%02d-%02d %02d:%02d:%02d UTC"),
                      tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
                      tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);
}
