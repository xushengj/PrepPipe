#include "EventLoopHelper.h"

int EventLoopHelper::execDialog(QDialog* dialog)
{
#ifdef PP_ENABLE_EVENTLOOP
    QEventLoop loop;
    QObject::connect(dialog, &QDialog::finished, &loop, &QEventLoop::quit);
    dialog->show();
    loop.exec();
#else
    volatile bool isDone = false;
    QObject::connect(dialog, &QDialog::finished, [&]()->void{isDone = true;});
    dialog->show();
    while (!isDone) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
#endif
    return dialog->result();
}

#ifndef PP_ENABLE_EVENTLOOP
EventLoopHelper::EventLoop::EventLoop()
    : QObject(nullptr)
{

}

void EventLoopHelper::EventLoop::quit()
{
    exitCode = 0;
    isDone = true;
}

void EventLoopHelper::EventLoop::exit(int code)
{
    exitCode = code;
    isDone = true;
}

int	EventLoopHelper::EventLoop::exec()
{
    while (!isDone) {
        QApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    return exitCode;
}
#endif /* PP_ENABLE_EVENTLOOP */
