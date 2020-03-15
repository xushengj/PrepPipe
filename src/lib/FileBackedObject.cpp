#include "src/lib/FileBackedObject.h"
#include "src/lib/IntrinsicObject.h"
#include "src/lib/ImportedObject.h"
#include <QFileInfo>
#include <QByteArray>
#include <QMessageBox>

QString FileBackedObject::getFileNameFilter() const {
    return tr("All files (*.*)");
}

FileBackedObject* FileBackedObject::open(const QString& filePath, QWidget* window)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(window,
                              tr("File open failed"),
                              tr("Failed to open the specified file: %1").arg(filePath));
        return nullptr;
    }
    QByteArray ba = f.readAll();
    f.close();

    QFileInfo file(filePath);
    if (file.suffix() == "xml") {
        QXmlStreamReader xml(ba);
        if (IntrinsicObject* obj = IntrinsicObject::loadFromXML(xml)) {
            obj->setFilePath(filePath);
            return obj;
        }
    }

    ImportedObject* obj = ImportedObject::open(ba, window);
    if (obj) {
        obj->setFilePath(filePath);
        obj->setName(file.fileName());
    }
    return obj;
}

bool FileBackedObject::saveToFile(QWidget* dialogParent)
{
    if (saveToFile()) {
        return true;
    }
    QMessageBox::critical(dialogParent,
                          tr("Save file failed"),
                          tr("Failed to save data to %1").arg(getFilePath()));
    return false;
}
