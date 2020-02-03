#include "ExecuteObject.h"

ExecuteObject::ExecuteObject(ObjectType ty, const ConstructOptions &opt)
    : ObjectBase(ty, opt)
{

}

void ExecuteObject::setInput(QString inputName, ObjectBase* obj)
{
    Q_UNUSED(inputName)
    Q_UNUSED(obj)
    qFatal("No matching input");
}

void ExecuteObject::setInput(QString inputName, QList<ObjectBase*> batch)
{
    Q_UNUSED(inputName)
    Q_UNUSED(batch)
    qFatal("No matching input");
}

void ExecuteObject::start()
{
    emit started();
    ExitCause cause = ExitCause::Completed;
    int status = startImpl(cause);
    emit finished(status, cause);
}
