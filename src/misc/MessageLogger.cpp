#include "MessageLogger.h"

#include <QDebug>
#include <QDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QUrl>
#include <QThreadStorage>

#include <cstdio>

MessageLogger* MessageLogger::ptr = nullptr;

MessageLogger::MessageLogger(QObject *parent) : QObject(parent)
{
    timer.start();
    bootstrapLog.reserve(1024);
    initFailureHandler();
}

MessageLogger::~MessageLogger()
{
    Q_ASSERT(threadInfo.isEmpty());

    if (isFatalEventOccurred) {
        logFile.setAutoRemove(false);
    } else {
        logFile.remove();
    }
}

void MessageLogger::createInstance()
{
    Q_ASSERT(!ptr);
    ptr = new MessageLogger(nullptr);
    qInstallMessageHandler(MessageLogger::firstStageMessageHandler);
}

void MessageLogger::destructInstance()
{
    Q_ASSERT(ptr);
    delete ptr;
    ptr = nullptr;
}

QString MessageLogger::getLogNameTemplate()
{
    return QDir::tempPath() + "/supp_" + QDateTime::currentDateTime().toString(Qt::ISODate).replace(':','_') + "_XXXXXX.log";
}

void MessageLogger::bootstrapFinished(QWidget* mainWindow)
{
    window = mainWindow;
    logFile.setFileTemplate(getLogNameTemplate());
    Q_ASSERT(logFile.open());
    qInfo() << "Log file opened at" << logFile.fileName();
    // dump all contents from bootstrap log
    QByteArray log = bootstrapLog.toUtf8();
    bootstrapLog.clear();
    logFile.write(log);
    qInstallMessageHandler(MessageLogger::normalMessageHandler);
}

namespace {
QThreadStorage<QTemporaryFile*> logDestination;
} // end of anonymous namespace


QIODevice* getLogDestination()
{
    if (Q_UNLIKELY(!logDestination.hasLocalData())) {
        QThread* t = QThread::currentThread();
        MessageLogger* logger = MessageLogger::inst();
        for (auto& info : logger->threadInfo){
            if (&info->thread == t)
                return &info->log;
        }
        return &logger->logFile;
    }
    return logDestination.localData();
}

void MessageLogger::openLogFile()
{
    logFile.flush();
    QDesktopServices::openUrl(QUrl::fromLocalFile(logFile.fileName()));
}

void MessageLogger::mainThreadFatalEventWrapup_Unsafe()
{
    openLogFile();
    QMessageBox::critical(window,
                          tr("Fatal error occured"),
                          tr("Sorry, a fatal error has occurred and the program cannot continue."
                             " If it is not due to insufficient memory or disk space, please submit a bug report.\n"
                             "The log is saved at ") + logFile.fileName());
}

QIODevice *MessageLogger::startExceptionInfoDump()
{
    QIODevice* dest = getLogDestination();
    writeExceptionHeader(dest);
    inst()->isFatalEventOccurred = true;
    return dest;
}

void MessageLogger::crashReportWrapup()
{
    // step 1: find current thread
    QThread* curThread = QThread::currentThread();
    for (auto& info : ptr->threadInfo) {
        if (&info->thread == curThread) {
            info->isFatalEventOccurred = true;
            info->log.flush();
            info->fatalEventCallback();
            // probably never reached.. but just in case
            // a return would probably result in process abort
            return;
        }
    }

    // callee will also flush the log file
    ptr->mainThreadFatalEventWrapup_Unsafe();
}

QThread* MessageLogger::createThread(const QString& description, std::function<void()> fatalEventCallback)
{
    ThreadInfo* infoPtr = new ThreadInfo;
    infoPtr->description = description;
    infoPtr->fatalEventCallback = fatalEventCallback;
    infoPtr->isFatalEventOccurred = false;
    infoPtr->log.setFileTemplate(getLogNameTemplate());
    Q_ASSERT(infoPtr->log.open());
    threadInfo.push_back(infoPtr);
    qInfo() << "Thread" << description << "is created with log file at" << infoPtr->log.fileName();
    return &infoPtr->thread;
}

void MessageLogger::cleanupThread(QThread* thread)
{
    Q_ASSERT(!thread->isRunning());
    for (auto iter = threadInfo.begin(), iterEnd = threadInfo.end(); iter != iterEnd; ++iter) {
        ThreadInfo* infoPtr = *iter;
        if (&infoPtr->thread == thread) {
            auto log = &infoPtr->log;
            qInfo() << "Cleaning up thread" << infoPtr->description << "with log file at" << log->fileName();
            if (infoPtr->isFatalEventOccurred) {
                // move all content to main log file
                qint64 size = log->size();
                qInfo () << "Fatal event occurred on given thread with following log: (size =" << size << ")";
                auto logPtr = log->map(0, size, QFileDevice::MapPrivateOption);
                logFile.write(reinterpret_cast<char*>(logPtr), size);
                log->unmap(logPtr);
                qInfo() << "End of log" << log->fileName();
            }
            log->close();
            log->remove();
            threadInfo.erase(iter);
            delete infoPtr;
            return;
        }
    }
    qFatal("thread for cleanup is not found");
}

#define HEADER_BUFFER_SIZE 64
namespace {

const char* getMsgType(QtMsgType type)
{
    const char* msgType = nullptr;
    switch(type) {
    case QtDebugMsg:
        msgType = "Debug:";
        break;
    case QtInfoMsg:
        msgType = "Info: ";
        break;
    case QtWarningMsg:
        msgType = "Warn: ";
        break;
    case QtCriticalMsg:
        msgType = "Crit: ";
        break;
    case QtFatalMsg:
        msgType = "Fatal:";
        break;
    }
    return msgType;
}
void setMsgHeader(char buffer[HEADER_BUFFER_SIZE], const char* msgType)
{
    qint64 elapsed = MessageLogger::inst()->getMSecSinceBootstrap();
    qint64 sec = elapsed / 1000;
    qint64 msec = elapsed % 1000;
    std::snprintf(buffer, HEADER_BUFFER_SIZE, "[%4lld.%03lld] %s ", sec, msec, msgType);
}

} // end of anonymous namespace

void MessageLogger::firstStageMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    char buffer[HEADER_BUFFER_SIZE];
    setMsgHeader(buffer, getMsgType(type));
    ptr->bootstrapLog.append(buffer);
    ptr->bootstrapLog.append(msg);
    if (context.file) {
        ptr->bootstrapLog.append(' ');
        ptr->bootstrapLog.append('(');
        ptr->bootstrapLog.append(context.file);
        std::snprintf(buffer, HEADER_BUFFER_SIZE, ":%d)\n", context.line);
        ptr->bootstrapLog.append(buffer);
    }

    // if a qFatal arrives, dump all immediately
    if (Q_UNLIKELY(type == QtFatalMsg)) {
        QByteArray msg8Bit = ptr->bootstrapLog.toLocal8Bit();
        std::fwrite(msg8Bit.constData(), 1, static_cast<std::size_t>(msg8Bit.length()), stderr);
    }
}

void MessageLogger::normalMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QIODevice* dest = getLogDestination();
    char buffer[HEADER_BUFFER_SIZE];
    setMsgHeader(buffer, getMsgType(type));
    dest->write(buffer);
    dest->write(msg.toUtf8());
    if (context.file) {
        dest->write(" (", 2);
        dest->write(context.file);
        std::snprintf(buffer, HEADER_BUFFER_SIZE, ":%d)\n", context.line);
        dest->write(buffer);
    }

    if (Q_UNLIKELY(type == QtFatalMsg)) {
        inst()->logFile.setAutoRemove(false);
        tryDumpStackTrace(dest);
        crashReportWrapup();
    }
}

void MessageLogger::writeExceptionHeader(QIODevice *dest)
{
    char buffer[HEADER_BUFFER_SIZE];
    setMsgHeader(buffer, "EXCEPTION:");
    dest->write(buffer);
}

