#include "ExecuteObject.h"

void ExecuteObject::setInput(QString inputName, ObjectBase* obj)
{
    Q_UNUSED(inputName)
    Q_UNUSED(obj)
    qFatal("No matching input");
}

void ExecuteObject::setInput(QString inputName, QList<ObjectBase*> batch)
{
    for (auto* ptr : batch) {
        setInput(inputName, ptr);
    }
}

void ExecuteObject::start()
{
    emit started();
    ExitCause cause = ExitCause::Completed;
    int status = startImpl(cause);
    emit finished(status, static_cast<int>(cause));
}
