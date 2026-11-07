#include "CrashSaver.h"
#include "TimeManager.h"
#include "../Main.h" // for VERSION_NUMBER and CUSTOM_APP_MODEL

#ifdef ESP8266

extern "C" void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end)
{
    // avoid to log a crash during Update reboot
    if (SystemState::shouldReboot)
        return;

    if (CrashSaver::_fs == nullptr)
        return;

    FS &fs = *CrashSaver::_fs;

    // open the file in appending mode
    File logFile = fs.open(CrashSaver::getNextLogFilePath(), "a");

    // if the file does not yet exist
    if (!logFile)
    {
        // open the file in write mode
        logFile = fs.open(CrashSaver::getNextLogFilePath(), "w");
    }

    if (!logFile)
        return;

    // if the file is (now) a valid file

    // maximum tmpBuffer size needed is 93 (UTC datetime crash line), so 100 should be enough
    char tmpBuffer[100];

    // log model and version info
    int writtenLen = snprintf_P(tmpBuffer, sizeof(tmpBuffer), PSTR("Model: %s\nVersion: %s\n"), CUSTOM_APP_MODEL, VERSION_NUMBER);
    if (writtenLen > 0)
        logFile.write(tmpBuffer, writtenLen);

    // log crash time, reason and exception cause
    int offset = TimeManager::getUTCTime(tmpBuffer, sizeof(tmpBuffer));
    if (offset > 0)
    {
        snprintf_P(tmpBuffer + offset, sizeof(tmpBuffer) - offset,
                   PSTR("\nRestart reason: %d\nException cause: %d\n"), rst_info->reason, rst_info->exccause);
    }
    else
        sprintf_P(tmpBuffer, PSTR("Crashed at %lu ms (no NTP)\nRestart reason: %d\nException cause: %d\n"), millis(), rst_info->reason, rst_info->exccause);
    logFile.write(tmpBuffer, strlen(tmpBuffer));

    // log crash info (epc1, epc2, epc3, excvaddr, depc) and stack trace
    // 83 chars of epc1, epc2, epc3, excvaddr, depc info + 13 chars of >stack>
    sprintf_P(tmpBuffer, PSTR("epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x depc=0x%08x\n>>>stack>>>\n"),
              rst_info->epc1, rst_info->epc2, rst_info->epc3, rst_info->excvaddr, rst_info->depc);
    logFile.write(tmpBuffer, strlen(tmpBuffer));

    uint16_t stackLength = stack_end - stack;

    // log stack trace
    // one loop contains 45 chars of stack address and its content
    // e.g. "3fffffb0: feefeffe feefeffe 3ffe8508 40100459"
    for (uint16_t i = 0; i < stackLength; i += 0x10)
    {
        uint32_t *p0 = (uint32_t *)(stack + i + 0);
        uint32_t *p1 = (uint32_t *)(stack + i + 4);
        uint32_t *p2 = (uint32_t *)(stack + i + 8);
        uint32_t *p3 = (uint32_t *)(stack + i + 12);

        writtenLen = snprintf_P(tmpBuffer, sizeof(tmpBuffer), PSTR("%08x: %08x %08x %08x %08x\n"), stack + i, *p0, *p1, *p2, *p3);

        if (writtenLen > 0)
            logFile.write(tmpBuffer, writtenLen);
    }
    logFile.write("<<<stack<<<\n\n");
    logFile.close();
}

FS *CrashSaver::_fs = nullptr;

char CrashSaver::_nextLogFilePath[LOG_FILE_PATH_LEN] = {0};

void CrashSaver::init(FS &fs)
{
    _fs = &fs;
    pruneOldLogs();
    calculateNextLogFilePath();
}

uint16_t CrashSaver::parseLogFileNumber(const String &fileName)
{
    // only names made of 1 to 5 digits, with a value from 1 to 65535
    if (fileName.length() == 0 || fileName.length() > 5)
        return 0;

    uint32_t number = 0;
    for (size_t i = 0; i < fileName.length(); i++)
    {
        char c = fileName[i];
        if (c < '0' || c > '9')
            return 0;
        number = number * 10 + (c - '0');
    }

    return (number <= UINT16_MAX) ? number : 0;
}

void CrashSaver::scanLogFiles(uint16_t &logCount, uint16_t &lowestNumber, uint16_t &highestNumber)
{
    logCount = 0;
    lowestNumber = 0;
    highestNumber = 0;

    if (_fs == nullptr)
        return;

    Dir dir = _fs->openDir(DEFAULT_DIR);
    while (dir.next())
    {
        uint16_t number = parseLogFileNumber(dir.fileName());
        if (number == 0)
            continue; // not a crash log: never counted, never removed

        logCount++;
        if (lowestNumber == 0 || number < lowestNumber)
            lowestNumber = number;
        if (number > highestNumber)
            highestNumber = number;
    }
}

size_t CrashSaver::getFreeBytes()
{
    FSInfo fsInfo;
    if (_fs == nullptr || !_fs->info(fsInfo))
        return SIZE_MAX; // unknown: don't remove logs because of free space
    return (fsInfo.totalBytes > fsInfo.usedBytes) ? fsInfo.totalBytes - fsInfo.usedBytes : 0;
}

void CrashSaver::pruneOldLogs()
{
    uint16_t logCount, lowestNumber, highestNumber;

    // remove the oldest log (lowest number) until both limits are respected
    // (each pass removes one file, so this loop runs at most once per existing log)
    while (true)
    {
        scanLogFiles(logCount, lowestNumber, highestNumber);
        if (logCount == 0)
            break;

        bool tooManyLogs = (logCount >= MAX_LOG_FILES); // keep room for the next crash
        bool lowFreeSpace = (getFreeBytes() < MIN_FREE_BYTES);
        if (!tooManyLogs && !lowFreeSpace)
            break;

        char oldestPath[LOG_FILE_PATH_LEN];
        snprintf(oldestPath, sizeof(oldestPath), "%s%u", DEFAULT_DIR, lowestNumber);
        if (!_fs->remove(oldestPath))
            break; // can't remove: stop rather than loop forever

        yield();
    }
}

void CrashSaver::calculateNextLogFilePath()
{
    // The next log is numbered after the HIGHEST existing number, not after the number of files:
    // once old logs are removed, numbers have gaps (e.g. 3, 4, 5, 6 remain), and "count + 1" (5) would point
    // to an existing log, which the crash handler would then append to.
    // Numbers only grow, so the highest number is always the most recent crash.
    uint16_t logCount, lowestNumber, highestNumber;
    scanLogFiles(logCount, lowestNumber, highestNumber);

    uint16_t nextNumber = (highestNumber < UINT16_MAX) ? highestNumber + 1 : UINT16_MAX;

    snprintf(_nextLogFilePath, sizeof(_nextLogFilePath), "%s%u", DEFAULT_DIR, nextNumber);
}

uint16_t CrashSaver::count()
{
    if (_fs == nullptr)
        return 0;

    uint16_t fileCount = 0;
    Dir dir = _fs->openDir(DEFAULT_DIR);
    while (dir.next())
    {
        if (fileCount < UINT16_MAX)
            fileCount++;
    }

    return fileCount;
}

void CrashSaver::iterateCrashLogFiles(std::function<void(uint16_t, const char *)> callback)
{
    if (_fs == nullptr || !callback)
        return;

    Dir dir = _fs->openDir(DEFAULT_DIR);
    uint16_t fileNumber = 0;
    while (dir.next())
    {
        if (fileNumber < UINT16_MAX)
            fileNumber++;

        char fullPath[LOG_FILE_PATH_LEN] = {0};
        strncpy(fullPath, DEFAULT_DIR, sizeof(fullPath) - 1);
        strncat(fullPath, dir.fileName().c_str(), sizeof(fullPath) - strlen(fullPath) - 1);

        callback(fileNumber, fullPath);
        yield();
    }
}

bool CrashSaver::clearAllLogs()
{
    if (_fs == nullptr)
        return false;

    bool allRemoved = true;
    Dir dir = _fs->openDir(DEFAULT_DIR);
    while (dir.next())
    {
        char fullPath[LOG_FILE_PATH_LEN] = {0};
        strncpy(fullPath, DEFAULT_DIR, sizeof(fullPath) - 1);
        strncat(fullPath, dir.fileName().c_str(), sizeof(fullPath) - strlen(fullPath) - 1);

        if (!_fs->remove(fullPath))
            allRemoved = false;
    }

    calculateNextLogFilePath();

    return allRemoved;
}

#endif // ESP8266
