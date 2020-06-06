#include "src/lib/FileBackedObject.h"
#include "src/lib/IntrinsicObject.h"
#include "src/lib/ImportedObject.h"
#include <QFileInfo>
#include <QByteArray>
#include <QMessageBox>
#include <QFileDialog>

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
            obj->setName(file.baseName());
            obj->setFilePath(filePath);
            return obj;
        }
    }

    ImportedObject* obj = ImportedObject::open(ba, window);
    if (obj) {
        obj->setFilePath(filePath);
        obj->setName(file.baseName());
    }
    return obj;
}

bool FileBackedObject::saveToFileStorage(QWidget* dialogParent, QString startDir)
{
    if (getFilePath().isEmpty()) {
        QString start = startDir;
        QString objName = getName();
        if (!objName.isEmpty()) {
            start.append('/');
            start.append(objName);
        }
        QString filePath = QFileDialog::getSaveFileName(dialogParent, tr("Save file"), start, getFileNameFilter());
        if (filePath.isEmpty())
            return false;
        setFilePath(filePath);
    }
    if (saveToFile()) {
        return true;
    }
    QMessageBox::critical(dialogParent,
                          tr("Save file failed"),
                          tr("Failed to save data to %1. Please check if file permission is correctly set and there are sufficient disk space.").arg(getFilePath()));
    return false;
}
