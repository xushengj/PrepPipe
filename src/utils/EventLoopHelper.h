#ifndef EVENTLOOPHELPER_H
#define EVENTLOOPHELPER_H

#include <QApplication>
#include <QObject>
#include <QDialog>
#include <QEventLoop>

/// This namespace contains utilities to work around the limitation on WebAssembly that nested event loops
/// are not supported.
namespace EventLoopHelper
{
    /// alternative to dialog->exec(). Note that the dialog will be non-modal.
    int execDialog(QDialog* dialog);

#ifdef PP_ENABLE_EVENTLOOP
    using EventLoop = QEventLoop;
#else
    class EventLoop : public QObject
    {
        Q_OBJECT
    public:
        EventLoop();
        int	exec();
        void exit(int returnCode = 0);

    public slots:
        void quit();

    private:
        int exitCode = 0;
        volatile bool isDone = false;
    };
#endif
};

#endif // EVENTLOOPHELPER_H
