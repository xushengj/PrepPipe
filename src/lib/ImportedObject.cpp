#include "ImportedObject.h"
#include "src/gui/ImportFileDialog.h"
#include "src/utils/EventLoopHelper.h"
#include <functional>
#include <QSaveFile>

namespace {
std::vector<const FileImportSupportDecl*>* instanceVecPtr = nullptr;
}

FileImportSupportDecl::FileImportSupportDecl()
{
    if (!instanceVecPtr) {
        instanceVecPtr = new std::vector<const FileImportSupportDecl*>;
    }
    instanceVecPtr->push_back(this);
}

const std::vector<const FileImportSupportDecl*>& FileImportSupportDecl::getInstanceVec()
{
    Q_ASSERT(instanceVecPtr);
    return *instanceVecPtr;
}

ImportedObject* ImportedObject::open(const QByteArray &src, QWidget* window)
{
    ImportFileDialog* dialog = new ImportFileDialog(window);
    dialog->setSrc(src);
    if (EventLoopHelper::execDialog(dialog) == QDialog::Accepted) {
        return dialog->getResult();
    }
    return nullptr;
}

bool ImportedObject::saveToFile()
{
    QString path = getFilePath();
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    QByteArray ba;
    if (!save(ba)) {
        return false;
    }
    f.write(ba);
    return f.commit();
}
