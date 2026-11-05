#ifndef TimeManager_h
#define TimeManager_h

#include <stdint.h>
#include <Arduino.h>
#include <time.h>
#include <sys/time.h>

class TimeManager
{
public:
    TimeManager() = delete;

    // Initialize time synchronization with NTP server
    static void init(const char *ntpServer = "pool.ntp.org");

    // Get the current NTP epoch (seconds since Unix epoch)
    static uint32_t getNtpEpoch() { return _ntpEpoch; }

    // Get the milliseconds value when NTP time was synchronized
    static uint32_t getNtpMillis() { return _ntpMillis; }

    // Get the current time in seconds since Unix epoch
    static uint32_t getCurrentEpoch();

    // Check if NTP time has been synchronized
    static bool isNtpSynchronized() { return _ntpEpoch != 0; }

    // Format epoch time as UTC datetime string (buf must be at least 24 chars)
    // Format: "YYYY-MM-DD HH:MM:SS UTC"
    // Returns number of characters written (not including null terminator)
    static int formatEpochToUTC(uint32_t epoch, char *buf, size_t bufSize);

private:
    static uint32_t _ntpEpoch;
    static uint32_t _ntpMillis;

    friend class CrashSaver;
};

#endif // TimeManager_h
