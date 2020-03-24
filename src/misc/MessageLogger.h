#ifndef MESSAGELOGGER_H
#define MESSAGELOGGER_H

#include <QObject>
#include <QElapsedTimer>
#include <QTemporaryFile>
#include <QBuffer>
#ifndef SUPP_NO_THREADS
#include <QThread>
#include <QLinkedList>
#endif

class MessageLogger : public QObject
{
    Q_OBJECT
public:
    static MessageLogger* inst() {return ptr;}
    static void createInstance();
    static void destructInstance();
    static void firstStageMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void normalMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void bootstrapFinished(QWidget* mainWindow);

#ifndef SUPP_NO_THREADS
    struct ThreadInfo {
        QThread thread;
        QString description;
        QTemporaryFile log;
        bool isFatalEventOccurred = false;
        std::function<void()> fatalEventCallback;
    };

    // these two functions MUST only be called from main thread and NOT be called when there are other threads running
    // otherwise threadInfo will be corrupted
    QThread* createThread(const QString& description, std::function<void()> fatalEventCallback);
    void cleanupThread(QThread* thread);
#endif

    qint64 getMSecSinceBootstrap() const {
        return timer.elapsed();
    }

public slots:
    void openLogFile();

signals:
    void messageUpdated();

public:
    // only for platform dependent error handling code
    static QIODevice *startExceptionInfoDump();
    static void crashReportWrapup(); // this may or may not return

private:
    explicit MessageLogger(QObject *parent = nullptr);
    virtual ~MessageLogger() override;
    void mainThreadFatalEventWrapup_Unsafe();
    static void writeExceptionHeader(QIODevice* dest);
    static QString getLogNameTemplate();

    // platform dependent
    static void tryDumpStackTrace(QIODevice* dest);
    void initFailureHandler();

private:
    static MessageLogger* ptr;

private:
    QElapsedTimer timer;
    QString bootstrapLog;
    QWidget* window = nullptr;
    QTemporaryFile logFile;
    bool isFatalEventOccurred = false;
#ifndef SUPP_NO_THREADS
    QList<ThreadInfo*> threadInfo;
#endif

friend QIODevice* getLogDestination();
};

#endif // MESSAGELOGGER_H
