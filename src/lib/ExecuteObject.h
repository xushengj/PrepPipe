#ifndef EXECUTEOBJECT_H
#define EXECUTEOBJECT_H

#include "src/lib/ObjectBase.h"

#include <QObject>

class ExecuteObject : public ObjectBase
{
    Q_OBJECT
public:
    ExecuteObject(ObjectType ty, const ConstructOptions& opt);

    virtual void setInput(QString inputName, ObjectBase* obj);
    virtual void setInput(QString inputName, QList<ObjectBase*> batch);

    virtual int startImpl() = 0;

signals:
    void finished(int status);

public slots:
    void start();
};

#endif // EXECUTEOBJECT_H
