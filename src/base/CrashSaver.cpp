#include "CrashSaver.h"

extern "C" void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end)
{
    if (CrashSaver::_fs == nullptr)
    {
        return;
    }

    FS &fs = *CrashSaver::_fs;

    uint32_t crashTime = millis();

    // open the file in appending mode
    File logFile = fs.open(CrashSaver::getNextLogFilePath(), "a");

    // if the file does not yet exist
    if (!logFile)
    {
        // open the file in write mode
        logFile = fs.open(CrashSaver::getNextLogFilePath(), "w");
    }

    if (!logFile)
    {
        return;
    }
    // if the file is (now) a valid file

    // maximum tmpBuffer size needed is 83, so 100 should be enough
    char tmpBuffer[100];

    // max. 65 chars of Crash time, reason, exception
    sprintf_P(tmpBuffer, PSTR("Crashed at %d ms\nRestart reason: %d\nException cause: %d\n"), crashTime, rst_info->reason, rst_info->exccause);
    logFile.write(tmpBuffer, strlen(tmpBuffer));

    // 83 chars of epc1, epc2, epc3, excvaddr, depc info + 13 chars of >stack>
    sprintf_P(tmpBuffer, PSTR("epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x depc=0x%08x\n>>>stack>>>\n"),
              rst_info->epc1, rst_info->epc2, rst_info->epc3, rst_info->excvaddr, rst_info->depc);
    logFile.write(tmpBuffer, strlen(tmpBuffer));

    uint16_t stackLength = stack_end - stack;

    // collect stack trace
    // one loop contains 45 chars of stack address and its content
    // e.g. "3fffffb0: feefeffe feefeffe 3ffe8508 40100459"
    for (uint16_t i = 0; i < stackLength; i += 0x10)
    {
        uint32_t *p0 = (uint32_t *)(stack + i + 0);
        uint32_t *p1 = (uint32_t *)(stack + i + 4);
        uint32_t *p2 = (uint32_t *)(stack + i + 8);
        uint32_t *p3 = (uint32_t *)(stack + i + 12);

        int writtenLen = snprintf_P(
            tmpBuffer,
            sizeof(tmpBuffer),
            PSTR("%08x: %08x %08x %08x %08x\n"),
            stack + i,
            *p0,
            *p1,
            *p2,
            *p3);

        if (writtenLen > 0)
        {
            logFile.write(tmpBuffer, writtenLen);
        }
    }
    logFile.write("<<<stack<<<\n\n");
    logFile.close();
}

FS *CrashSaver::_fs = nullptr;

char CrashSaver::_nextLogFilePath[128] = {0};

void CrashSaver::init(FS &fs)
{
    _fs = &fs;
    calculateNextLogFilePath();
}

void CrashSaver::calculateNextLogFilePath()
{
    uint16_t nextFileIndex = count();
    if (nextFileIndex < UINT16_MAX)
    {
        nextFileIndex++;
    }

    snprintf(_nextLogFilePath, sizeof(_nextLogFilePath), "%s%u", DEFAULT_DIR, nextFileIndex);
}

uint16_t CrashSaver::count()
{
    if (_fs == nullptr)
    {
        return 0;
    }

    uint16_t fileCount = 0;
    Dir dir = _fs->openDir(DEFAULT_DIR);
    while (dir.next())
    {
        if (fileCount < UINT16_MAX)
        {
            fileCount++;
        }
    }

    return fileCount;
}

void CrashSaver::iterateCrashLogFiles(std::function<void(uint16_t, const char *)> callback)
{
    if (!callback || _fs == nullptr)
    {
        return;
    }

    Dir dir = _fs->openDir(DEFAULT_DIR);
    uint16_t fileNumber = 0;
    while (dir.next())
    {
        if (fileNumber < UINT16_MAX)
        {
            fileNumber++;
        }
        const String fileName = dir.fileName();
        callback(fileNumber, fileName.c_str());
        yield();
    }
}

bool CrashSaver::clearAllLogs()
{
    if (_fs == nullptr)
    {
        return false;
    }

    bool allRemoved = true;
    Dir dir = _fs->openDir(DEFAULT_DIR);
    while (dir.next())
    {
        const String fileName = dir.fileName();
        if (!_fs->remove(fileName))
        {
            allRemoved = false;
        }
    }

    calculateNextLogFilePath();

    return allRemoved;
}
