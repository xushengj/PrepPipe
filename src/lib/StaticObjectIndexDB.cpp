#include "StaticObjectIndexDB.h"

StaticObjectIndexDB* StaticObjectIndexDB::ptr = nullptr;

StaticObjectIndexDB::StaticObjectIndexDB() : QObject(nullptr)
{
    // TODO
}

ObjectBase* StaticObjectIndexDB::getObject(const QStringList& nameSpace, const QString& name)
{
    // TODO
    Q_UNUSED(nameSpace)
    Q_UNUSED(name)
    return nullptr;
}

void StaticObjectIndexDB::createInstance()
{
    // TODO
}

void StaticObjectIndexDB::destructInstance()
{
    // TODO
}
