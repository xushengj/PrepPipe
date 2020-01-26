#ifndef MESSAGELOGGER_H
#define MESSAGELOGGER_H

#include <QObject>
#include <QElapsedTimer>
#include <QTemporaryFile>
#include <QBuffer>

class MessageLogger : public QObject
{
    Q_OBJECT
public:
    static MessageLogger* inst() {return ptr;}
    static void createInstance();
    static void destructInstance();
    static void firstStageMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void normalMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void bootstrapFinished(QWidget* mainWindow);

    qint64 getMSecSinceBootstrap() const {
        return timer.elapsed();
    }

public slots:
    void openLogFile();

signals:
    void messageUpdated();

public:
    // only for platform dependent error handling code
    static QTemporaryFile* startExceptionInfoDump();
    static void crashReportWrapup();

private:
    explicit MessageLogger(QObject *parent = nullptr);
    void handleFatalMessage();
    void handleFatal_Unsafe();
    void writeExceptionHeader();

    // platform dependent
    void tryDumpStackTrace();
    void initFailureHandler();

private:
    static MessageLogger* ptr;

private:
    QElapsedTimer timer;
    QString bootstrapLog;
    QWidget* window = nullptr;
    QTemporaryFile logFile;
};

#endif // MESSAGELOGGER_H
