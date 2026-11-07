/*

Inspired by https://github.com/brainelectronics/EspSaveCrashSpiffs

*/

#ifndef CrashSaver_h
#define CrashSaver_h

#ifdef ESP8266

#include <FS.h>
#include <functional>
#include <user_interface.h>

#include "SystemState.h"
#include "TimeManager.h"

class CrashSaver
{
public:
    static constexpr const char *DEFAULT_DIR = "/crash/";
    static constexpr unsigned LOG_FILE_PATH_LEN = 13; // "/crash/" + "65535" + '\0'

    // Crash log retention.
    // Each crash log holds a full stack dump (up to about 10 KB). Without a limit, a crash loop (e.g. a crash at
    // every boot) keeps adding files until the filesystem is full; configuration files can then no longer be saved.
    // At boot, init() removes the oldest logs (lowest numbers) so that:
    //  - at most MAX_LOG_FILES - 1 logs remain, leaving room for the next crash (MAX_LOG_FILES logs at most);
    //  - at least MIN_FREE_BYTES stay free on the filesystem, if removing logs can achieve it.
    // Pruning is done at boot, never in the crash handler, which must do as little as possible.
    static constexpr uint16_t MAX_LOG_FILES = 5;
    static constexpr size_t MIN_FREE_BYTES = 24 * 1024;

    static FS *_fs;

    CrashSaver() = delete;

    static const char *getNextLogFilePath() { return _nextLogFilePath; }
    static void init(FS &fs);

    static uint16_t count();
    static void iterateCrashLogFiles(std::function<void(uint16_t, const char *)> callback);
    static bool clearAllLogs();

private:
    // log files are named DEFAULT_DIR + number (1 to 65535); returns 0 for any other name
    static uint16_t parseLogFileNumber(const String &fileName);
    // number of log files, and lowest/highest log numbers (0 when there is no log)
    static void scanLogFiles(uint16_t &logCount, uint16_t &lowestNumber, uint16_t &highestNumber);
    static size_t getFreeBytes();
    static void pruneOldLogs();
    static void calculateNextLogFilePath();
    static char _nextLogFilePath[LOG_FILE_PATH_LEN];
};

#endif // ESP8266
#endif // CrashSaver_h
