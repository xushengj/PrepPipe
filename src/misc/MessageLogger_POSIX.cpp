#define _POSIX_C_SOURCE 200809L
#include "src/misc/MessageLogger.h"

#include <execinfo.h>
#include <errno.h>
#include <signal.h>

#include <QDebug>

namespace {

#define WRITE_LITERAL(str, dest) dest->write(str, sizeof(str)-1)

#define BACKTRACE_ENTRY_MAX 128
void* backtraceBuffer[BACKTRACE_ENTRY_MAX];

#define BUFFER_SIZE 32
void dumpStackTrace(QIODevice* dest)
{
    int numFrame = backtrace(backtraceBuffer, BACKTRACE_ENTRY_MAX);
    char** symbols = backtrace_symbols(backtraceBuffer, numFrame);
    char buffer[BUFFER_SIZE];
    if (!symbols) {
        // only return addresses are available
        char* err = strerror(errno);
        WRITE_LITERAL("backtrace_symbols() error: ", dest);
        dest->write(err);
        dest->write("\n", 1);
    } else {
        for (int i = 0; i < numFrame; ++i) {
            snprintf(buffer, sizeof(buffer), "frame %d: ", i);
            dest->write(buffer);
            dest->write(symbols[i]);
            dest->write("\n", 1);
        }
        free(symbols);
    }
}

void signalHandler(int signo, siginfo_t *info, void *ucontext)
{
    // we cannot make use of ucontext without breaking too much portability..
    Q_UNUSED(ucontext)
    auto* dest = MessageLogger::startExceptionInfoDump();
    char buffer[BUFFER_SIZE];
    switch (signo) {
    default: {
        snprintf(buffer, sizeof(buffer), "%d\n", signo);
        dest->write(buffer);
    }break;
#define HANDLE_EX(ex) case ex: WRITE_LITERAL(#ex "\n", dest); break;
    HANDLE_EX(SIGINT)
    HANDLE_EX(SIGTERM)
#undef HANDLE_EX
#define HANDLE_EX(ex) case ex: WRITE_LITERAL(#ex " (si_addr = ", dest); snprintf(buffer, sizeof(buffer), "%p)\n", info->si_addr); dest->write(buffer); break;
    HANDLE_EX(SIGSEGV)
    HANDLE_EX(SIGILL)
    HANDLE_EX(SIGFPE)
    HANDLE_EX(SIGBUS)
#undef HANDLE_EX
    }
    dumpStackTrace(dest);
    MessageLogger::crashReportWrapup();

    // we have to abort the program to prevent the loop of
    // trap -> signal handler -> done -> re-execute -> trap
    abort();
}

void sighupHandler(int signo)
{
    // just write a log and done
    Q_UNUSED(signo)
    qInfo() << "SIGHUP received";
}

} // end of anonymous namespace

void MessageLogger::tryDumpStackTrace()
{
    logFile.write("Trying to get POSIX Stack Trace:\n");
    dumpStackTrace(&logFile);
}

void MessageLogger::initFailureHandler()
{
    struct sigaction sig_action = {};
    sig_action.sa_sigaction = signalHandler;
    sig_action.sa_flags = SA_SIGINFO;

    sigaction(SIGSEGV,  &sig_action, nullptr);
    sigaction(SIGILL,   &sig_action, nullptr);
    sigaction(SIGFPE,   &sig_action, nullptr);
    sigaction(SIGBUS,   &sig_action, nullptr);
    sigaction(SIGINT,   &sig_action, nullptr);
    sigaction(SIGTERM,  &sig_action, nullptr);

    struct sigaction hup_action = {};
    hup_action.sa_handler = sighupHandler;

    sigaction(SIGHUP,   &hup_action, nullptr);
}
