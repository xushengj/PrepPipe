#ifndef EXECUTEOBJECT_H
#define EXECUTEOBJECT_H

#include "src/lib/ObjectBase.h"

#include <QObject>
#include <QThread>

class ExecuteObject : public ObjectBase
{
    Q_OBJECT
public:
    enum class ExitCause {
        Completed, // either normal execution or there is a (task-specific) runtime error
        FatalEvent, // a qFatal() or Q_ASSERT() or whatever
        Terminated // user requested termination
    };
    ExecuteObject(ObjectType ty, const ConstructOptions& opt);

    virtual void getChildExecuteObjects(QList<ExecuteObject*>& list) {Q_UNUSED(list)}

    virtual void setInput(QString inputName, ObjectBase* obj);
    virtual void setInput(QString inputName, QList<ObjectBase*> batch);

    virtual int startImpl(ExitCause& cause) = 0;

signals:
    void started();
    void finished(int status, ExitCause cause);
    void outputAvailable(ObjectBase* obj);
    void temporaryOutputFinishedUse(ObjectBase* obj);
    void statusUpdate(const QString& description, int start, int end, int value);

public slots:
    void start();

protected:
    bool isTerminationRequested(ExitCause& cause) {
        if (Q_UNLIKELY(QThread::currentThread()->isInterruptionRequested())) {
            cause = ExitCause::Terminated;
            return true;
        }
        return false;
    }
};

#endif // EXECUTEOBJECT_H
