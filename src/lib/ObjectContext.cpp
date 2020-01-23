#include "ObjectContext.h"
#include "src/lib/IntrinsicObject.h"

#include <QDebug>
#include <QDir>

void ObjectContext::clear()
{
    for (auto ptr : objects) {
        delete ptr;
    }
    objects.clear();
    objectMap.clear();
}

ObjectContext::ObjectContext(const QString& directory)
    : mainDirectory(directory)
{
    loadAllObjectsFromDirectory();
}

void ObjectContext::setDirectory(const QString& newDirectory)
{
    if (mainDirectory == newDirectory)
        return;

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
        ObjectBase::ConstructOptions opt;
        opt.filePath = absPath;
        opt.name = f.completeBaseName();
        opt.isLocked = !f.isWritable();
        QXmlStreamReader xml(&file);
        IntrinsicObject* obj = IntrinsicObject::loadFromXML(xml, opt);
        if (!obj) {
            qInfo() << absPath << "open as intrinsic object failed";
            continue;
        }
        int objIndex = objects.size();
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

ObjectBase* ObjectContext::getObject(const QString& name) const
{
    auto iter = objectMap.find(name);
    if (iter != objectMap.end()) {
        return iter.value();
    }
    return nullptr;
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
}
