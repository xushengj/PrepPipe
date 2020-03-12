#ifndef FILEBACKEDOBJECT_H
#define FILEBACKEDOBJECT_H

#include "src/lib/ObjectBase.h"

class FileBackedObject : public ObjectBase
{
    Q_OBJECT
public:
    explicit FileBackedObject(ObjectType ty)
        : ObjectBase(ty)
    {}
    FileBackedObject(const FileBackedObject& src) // we do not copy file path during cloning
        : ObjectBase(src), filePath()
    {}

    virtual ~FileBackedObject() override {}

    virtual QString getFilePath() const override final {
        return filePath;
    }

    void setFilePath(const QString& newFilePath) {
        filePath = newFilePath;
    }

private:
    QString filePath;

};

#endif // FILEBACKEDOBJECT_H
