#include "ImportedObject.h"
#include "src/gui/ImportFileDialog.h"
#include <functional>
#include <QSaveFile>

ImportedObject* ImportedObject::open(const QByteArray &src, QWidget* window)
{
    ImportFileDialog* dialog = new ImportFileDialog(window);
    dialog->setSrc(src);
    if (dialog->exec() == QDialog::Accepted) {
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
