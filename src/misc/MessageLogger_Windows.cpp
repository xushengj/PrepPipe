#include "src/misc/MessageLogger.h"

#include <windows.h>
#include <winnt.h>
#include <DbgHelp.h>

#include <cstdio>
#include <cstring>

#if !defined(_WIN64)
#error 32 bits Windows not supported!
#endif

class PlatformHelper {
public:
    static QTemporaryFile* startExceptionInfoDump();
    static void crashReportWrapup();
};

namespace {

#define WRITE_LITERAL(str, dest) dest->write(str, sizeof(str)-1)

// adapted from https://stackoverflow.com/questions/6205981/windows-c-stack-trace-from-a-running-app
#define BUFFER_SIZE 32
#define MAX_SYMBOL_LENGTH 2048
char IMAGEHLP_buffer[sizeof(IMAGEHLP_SYMBOL) + MAX_SYMBOL_LENGTH];
char symbolNameBuffer[MAX_SYMBOL_LENGTH];
void dumpStackTrace(HANDLE thread, PCONTEXT ContextRecord, QIODevice* dest)
{
    char buffer[BUFFER_SIZE];
#define ReportWinAPIError(func) \
do{                                                                     \
    DWORD error = GetLastError();                                       \
    WRITE_LITERAL(func " fail (Error ", dest);                          \
    snprintf(buffer, sizeof(buffer), "%lu)\n", error);                  \
    dest->write(buffer);                                                \
}while(0)

    HANDLE process = GetCurrentProcess();
    if (!SymInitialize(process, /* UserSearchPath */nullptr, /* fInvadeProcess */true)) {
        ReportWinAPIError("SymInitialize()");
        return;
    }

    // make sure we can close it
    class SymCleanupCaller {
        HANDLE p;
    public:
        SymCleanupCaller(HANDLE process)
            : p(process)
        {}
        ~SymCleanupCaller(){
            SymCleanup(p);
        }
    } dummy(process);

    SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    STACKFRAME64 s;
    s.AddrPC.Offset = ContextRecord->Rip;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrStack.Offset = ContextRecord->Rsp;
    s.AddrStack.Mode = AddrModeFlat;
    s.AddrFrame.Offset = ContextRecord->Rbp;
    s.AddrFrame.Mode = AddrModeFlat;

    unsigned frameIndex = 0;
    do {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,
                         process,
                         thread,
                         &s,
                         ContextRecord,
                         nullptr,
                         SymFunctionTableAccess64,
                         SymGetModuleBase64,
                         nullptr)) {
            ReportWinAPIError("StackWalk64()");
            return;
        }

        snprintf(buffer, sizeof(buffer), "frame %u: ", frameIndex);
        dest->write(buffer);
        frameIndex += 1;
        if (s.AddrPC.Offset == 0) {
            dest->write("No Symbols (PC == 0)\n");
        } else {
            // try to get symbol
            memset(IMAGEHLP_buffer, 0, sizeof(IMAGEHLP_buffer));
            IMAGEHLP_SYMBOL* sym = reinterpret_cast<IMAGEHLP_SYMBOL*>(IMAGEHLP_buffer);
            sym->SizeOfStruct = sizeof(sizeof(*sym));
            sym->MaxNameLength = sizeof(IMAGEHLP_buffer) - sizeof(*sym);
            DWORD64 displacement;
            if (!SymGetSymFromAddr64(process, s.AddrPC.Offset, &displacement, sym)) {
                ReportWinAPIError("SymGetSymFromAddr64()");
                return;
            }
            DWORD len = UnDecorateSymbolName(sym->Name, symbolNameBuffer, MAX_SYMBOL_LENGTH, UNDNAME_COMPLETE);
            if (len <= 0) {
                ReportWinAPIError("UnDecorateSymbolName()");
                return;
            }
            dest->write(symbolNameBuffer, len);
            snprintf(buffer, sizeof(buffer), " + 0x%zx (", displacement);
            dest->write(buffer);

            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(line);
            DWORD offsetFromSymbol = 0;
            if (!SymGetLineFromAddr64(process, s.AddrPC.Offset, &offsetFromSymbol, &line)) {
                dest->write("Error ", 6);
                snprintf(buffer, sizeof(buffer), "%lu)\n", GetLastError());
                dest->write(buffer);
            } else {
                snprintf(buffer, sizeof(buffer), ":%lu)\n", line.LineNumber);
                dest->write(line.FileName);
                dest->write(buffer);
            }
        }
    } while (s.AddrReturn.Offset != 0);
#undef ReportWinAPIError
}

// adapted from https://spin.atomicobject.com/2013/01/13/exceptions-stack-traces-c/
LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo);
LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
{
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    auto* dest = PlatformHelper::startExceptionInfoDump();
    switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
#define HANDLE_EX(ex) case ex: WRITE_LITERAL(#ex "\n", dest); break;
    HANDLE_EX(EXCEPTION_ACCESS_VIOLATION)
    HANDLE_EX(EXCEPTION_DATATYPE_MISALIGNMENT)
    HANDLE_EX(EXCEPTION_BREAKPOINT)
    HANDLE_EX(EXCEPTION_SINGLE_STEP)
    HANDLE_EX(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
    HANDLE_EX(EXCEPTION_FLT_DENORMAL_OPERAND)
    HANDLE_EX(EXCEPTION_FLT_DIVIDE_BY_ZERO)
    HANDLE_EX(EXCEPTION_FLT_INEXACT_RESULT)
    HANDLE_EX(EXCEPTION_FLT_INVALID_OPERATION)
    HANDLE_EX(EXCEPTION_FLT_OVERFLOW)
    HANDLE_EX(EXCEPTION_FLT_STACK_CHECK)
    HANDLE_EX(EXCEPTION_FLT_UNDERFLOW)
    HANDLE_EX(EXCEPTION_INT_DIVIDE_BY_ZERO)
    HANDLE_EX(EXCEPTION_INT_OVERFLOW)
    HANDLE_EX(EXCEPTION_PRIV_INSTRUCTION)
    HANDLE_EX(EXCEPTION_IN_PAGE_ERROR)
    HANDLE_EX(EXCEPTION_ILLEGAL_INSTRUCTION)
    HANDLE_EX(EXCEPTION_NONCONTINUABLE_EXCEPTION)
    HANDLE_EX(EXCEPTION_STACK_OVERFLOW)
    HANDLE_EX(EXCEPTION_INVALID_DISPOSITION)
    HANDLE_EX(EXCEPTION_GUARD_PAGE)
    HANDLE_EX(EXCEPTION_INVALID_HANDLE)
#undef HANDLE_EX
    default:
        WRITE_LITERAL("Unrecognized Exception\n", dest);
        break;
    }
    if (ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW) {
        dumpStackTrace(GetCurrentThread(), &ctx, dest);
    }
    PlatformHelper::crashReportWrapup();

    return EXCEPTION_EXECUTE_HANDLER;
}

} // end of anonymous namespace

QTemporaryFile *PlatformHelper::startExceptionInfoDump()
{
    MessageLogger::inst()->writeExceptionHeader();
    MessageLogger::inst()->logFile.setAutoRemove(false);
    return &MessageLogger::inst()->logFile;
}

void PlatformHelper::crashReportWrapup()
{
    // callee will also flush the log file
    MessageLogger::inst()->handleFatal_Unsafe();
}

void MessageLogger::tryDumpStackTrace()
{
    logFile.write("Trying to get Win64 Stack Trace:\n");
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    HANDLE thread = GetCurrentThread();
    dumpStackTrace(thread, &ctx, &logFile);
}

void MessageLogger::initFailureHandler()
{
    SetUnhandledExceptionFilter(windows_exception_handler);
}
