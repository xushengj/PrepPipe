#ifndef IMPORTFILEDIALOG_H
#define IMPORTFILEDIALOG_H

#include "src/lib/Tree/Tree.h"
#include "src/lib/Tree/Configuration.h"
#include "src/lib/ImportedObject.h"
#include "src/gui/ConfigurationInputWidget.h"

#include <QDialog>

namespace Ui {
class ImportFileDialog;
}

class ImportFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportFileDialog(QWidget *parent = nullptr);
    void setSrc(const QByteArray& srcBA);
    ~ImportFileDialog();

    ImportedObject* getResult() const {return result;}

private slots:
    void updateCurrentInputWidget();
    void tryOpen();

private:
    Ui::ImportFileDialog *ui;

    struct OpenTypeEntryType {
        ConfigurationData importConfig;
        QWidget* inputWidget = nullptr; // dummyPage (QWidget*) if no config; ConfigurationInputWidget* if there is
        bool isApplicable = false;
    };
    QVector<OpenTypeEntryType> openTypes;

    ImportedObject* result = nullptr;
    QByteArray src;
};

#endif // IMPORTFILEDIALOG_H
