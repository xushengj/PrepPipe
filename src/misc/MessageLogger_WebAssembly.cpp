#include "src/misc/MessageLogger.h"

void MessageLogger::tryDumpStackTrace(QIODevice* dest)
{
    dest->write("(Stack Trace Dump not supported for this platform)\n");
}

void MessageLogger::initFailureHandler()
{
    // Do nothing
}
