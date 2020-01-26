#ifndef MESSAGELOGGER_H
#define MESSAGELOGGER_H

#include <QObject>
#include <QElapsedTimer>
#include <QTemporaryFile>
#include <QBuffer>

class PlatformHelper;
class MessageLogger : public QObject
{
    Q_OBJECT

    friend class PlatformHelper;
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
