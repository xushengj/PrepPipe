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
    int status = startImpl();
    emit finished(status);
}
