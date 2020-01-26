#include "src/misc/MessageLogger.h"

void MessageLogger::tryDumpStackTrace()
{
    logFile.write("(Stack Trace Dump not supported for this platform)\n");
}

void MessageLogger::initFailureHandler()
{
    // Do nothing
}
