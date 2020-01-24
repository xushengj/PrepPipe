#include "MessageLogger.h"

#include <QDebug>
#include <QDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QUrl>

#include <cstdio>

MessageLogger* MessageLogger::ptr = nullptr;

MessageLogger::MessageLogger(QObject *parent) : QObject(parent)
{
    timer.start();
    bootstrapLog.reserve(1024);
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

void MessageLogger::bootstrapFinished(QWidget* mainWindow)
{
    window = mainWindow;
    QString logTemplate = QDir::tempPath() + "/supp_" + QDateTime::currentDateTime().toString(Qt::ISODate).replace(':','_') + "_XXXXXX.log";
    logFile.setFileTemplate(logTemplate);
    Q_ASSERT(logFile.open());
    qInfo() << "Log file opened at" << logFile.fileName();
    // dump all contents from bootstrap log
    QByteArray log = bootstrapLog.toUtf8();
    bootstrapLog.clear();
    logFile.write(log);
    qInstallMessageHandler(MessageLogger::normalMessageHandler);
}

void MessageLogger::openLogFile()
{
    logFile.flush();
    QDesktopServices::openUrl(QUrl::fromLocalFile(logFile.fileName()));
}

void MessageLogger::handleFatalMessage()
{
    logFile.setAutoRemove(false);
    logFile.close();
    openLogFile();
    QMessageBox::critical(window,
                          tr("Fatal error occured"),
                          tr("Sorry, a fatal error has occurred and the program cannot continue. The log is saved at:\n") + logFile.fileName());
}

#define HEADER_BUFFER_SIZE 64
namespace {

void setMsgHeader(char buffer[HEADER_BUFFER_SIZE], QtMsgType type) {
    qint64 elapsed = MessageLogger::inst()->getMSecSinceBootstrap();
    qint64 sec = elapsed / 1000;
    qint64 msec = elapsed % 1000;

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

    std::snprintf(buffer, HEADER_BUFFER_SIZE, "[%4zd.%03zd] %s ", sec,msec, msgType);
}
} // end of anonymous namespace

void MessageLogger::firstStageMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    char buffer[HEADER_BUFFER_SIZE];
    setMsgHeader(buffer, type);
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
    if (type == QtFatalMsg) {
        QByteArray msg8Bit = ptr->bootstrapLog.toLocal8Bit();
        std::fwrite(msg8Bit.constData(), 1, static_cast<std::size_t>(msg8Bit.length()), stderr);
    }
}

void MessageLogger::normalMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    inst()->handleMessage(type, context, msg);
}

void MessageLogger::handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    char buffer[HEADER_BUFFER_SIZE];
    setMsgHeader(buffer, type);
    logFile.write(buffer);
    logFile.write(msg.toUtf8());
    if (context.file) {
        logFile.write(" (", 2);
        logFile.write(context.file);
        std::snprintf(buffer, HEADER_BUFFER_SIZE, ":%d)\n", context.line);
        logFile.write(buffer);
    }

    if (type == QtFatalMsg) {
        handleFatalMessage();
    } else {
        emit messageUpdated();
    }
}


