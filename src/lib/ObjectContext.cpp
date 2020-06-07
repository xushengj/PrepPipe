#include "ObjectContext.h"
#include "src/lib/IntrinsicObject.h"
#include "src/lib/StaticObjectIndexDB.h"

#include <QDebug>
#include <QDir>

QHash<int, ObjectContext*> ObjectContext::ContextRecord;
int ObjectContext::ContextIndex = 0;

void ObjectContext::clear()
{
    for (auto ptr : objects) {
        delete ptr;
    }
    objects.clear();
    objectMap.clear();
    objectToRefIndexMap.clear();
    refIndexToObjectMap.clear();
}

ObjectContext::ObjectContext()
    : ctxIndex(ContextIndex++)
{
    ContextRecord.insert(ctxIndex, this);
}

ObjectContext::~ObjectContext()
{
    clear();
    ContextRecord.remove(ctxIndex);
}

ObjectContext::ObjectContext(const QString& directory)
    : ObjectContext()
{
    mainDirectory = directory;
    loadAllObjectsFromDirectory();
}

void ObjectContext::setDirectory(const QString& newDirectory)
{
    clear();
    mainDirectory = newDirectory;
    if (!newDirectory.isEmpty())
        loadAllObjectsFromDirectory();
}

void ObjectContext::loadAllObjectsFromDirectory()
{
    QDir workDirectory(mainDirectory, QStringLiteral("*.xml"));
    // if the directory contains something with .xml suffix, open all of them and add them to this namespace
    QFileInfoList fileList = workDirectory.entryInfoList(QDir::Files, QDir::Name | QDir::LocaleAware);
    qInfo() << fileList.size() << "xml files under directory" << workDirectory.absolutePath();
    for (const auto& f : fileList) {
        QString absPath = f.absoluteFilePath();
        QFile file(absPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qInfo() << absPath << "cannot be read";
            continue;
        }
        QXmlStreamReader xml(&file);
        IntrinsicObject* obj = IntrinsicObject::loadFromXML(xml);
        if (!obj) {
            qInfo() << absPath << "open as intrinsic object failed";
            continue;
        }
        obj->setName(f.completeBaseName());
        obj->setFilePath(absPath);

        objects.push_back(obj);

        // if the object is named, add to named object
        // however, if there is already an object with the same name in that namespace,
        // this object will be treated as if it is anonymous
        // because we sort the file list by name, this should result in consistent decision on which version get shadowed out
        // and user can exploit this to do simple versioning by copy and renaming files.
        QString objName = obj->getName();

        // well, if the object name is empty then forget this..
        if (objName.isEmpty())
            continue;

        if (ObjectBase* existingObj = getObject(objName)) {
            qInfo() << "Object" << objName << "at" << absPath << "is shadowed by the one at" << existingObj->getFilePath();
            continue;
        }

        objectMap.insert(objName, obj);
        qInfo() << objName << "at" << absPath << "registered";
    }
    // if any of the object is a DataManifestObject, also try to open all the files listed in that object
    // TODO
}

void ObjectContext::addObject(ObjectBase* obj)
{
    objects.push_back(obj);

    QString name = obj->getName();
    if (name.isEmpty() || objectMap.contains(name))
        return;

    objectMap.insert(name, obj);
}

void ObjectContext::releaseObject(ObjectBase* obj)
{
    int indexToRemove = objects.indexOf(obj);
    Q_ASSERT(indexToRemove >= 0);
    objects.removeAt(indexToRemove);

    QString name = obj->getName();
    if (!name.isEmpty()) {
        // update objectMap
        objectMap.remove(name);
        // if any existing object is shadowed by the removed object, place it into objectMap
        for (int i = 0, n = objects.size(); i < n; ++i) {
            ObjectBase* ptr = objects.at(i);
            if (ptr->getName() == name) {
                objectMap.insert(name, ptr);
                break;
            }
        }
    }

    // remove from references
    {
        auto iter = objectToRefIndexMap.find(obj);
        if (iter != objectToRefIndexMap.end()) {
            refIndexToObjectMap.remove(iter.value());
            objectToRefIndexMap.erase(iter);
        }
    }
}

int ObjectContext::getObjectReference(ObjectBase* obj)
{
    Q_ASSERT(objects.contains(obj));
    auto iter = objectToRefIndexMap.find(obj);
    if (iter != objectToRefIndexMap.end()) {
        return iter.value();
    }

    int index = nextReferenceIndex++;
    objectToRefIndexMap.insert(obj, index);
    refIndexToObjectMap.insert(index, obj);
    return index;
}

ObjectBase* ObjectContext::resolveNamedReference(const ObjectBase::NamedReference& ref, const QStringList& mainNameSpace, const ObjectContext* ctx)
{
    const QStringList& nameSpaceToUse = ref.nameSpace.isEmpty()? mainNameSpace : ref.nameSpace;
    if (nameSpaceToUse.isEmpty()) {
        if (Q_UNLIKELY(!ctx)) {
            qCritical() << "No main context to search for object" << ref.name;
            return nullptr;
        } else {
            ObjectBase* obj = ctx->getObject(ref.name);
            if (Q_UNLIKELY(!obj)) {
                qCritical() << "Object" << ref.name << "not found in" << ctx->getDirectory();
                return nullptr;
            }
            return obj;
        }
    } else {
        ObjectBase* obj = StaticObjectIndexDB::inst()->getObject(nameSpaceToUse, ref.name);
        if (Q_UNLIKELY(!obj)) {
            qCritical() << "Object" << ref.prettyPrint(mainNameSpace) << "not found";
            return nullptr;
        }
        return obj;
    }
}

ObjectBase* ObjectContext::resolveAnonymousReference(int ctxIndex, int refIndex)
{
    auto ctxIter = ContextRecord.find(ctxIndex);
    if (ctxIter == ContextRecord.end())
        return nullptr;
    ObjectContext* ctx = ctxIter.value();
    auto refIter = ctx->refIndexToObjectMap.find(refIndex);
    if (refIter == ctx->refIndexToObjectMap.end())
        return nullptr;
    return refIter.value();
}

