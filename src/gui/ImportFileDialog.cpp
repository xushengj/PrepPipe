#include "ImportFileDialog.h"
#include "ui_ImportFileDialog.h"

#include "src/lib/ImportedObject.h"
#include "src/lib/DataObject/PlainTextObject.h"

#include <QMessageBox>

namespace {

struct ImportedObjectEntryType {
    QString fileTypeDisplayName;
    std::function<bool(const QByteArray&, ConfigurationData&)> contentCheckCB; // return true if the file may be opened as this object type
    std::function<const ConfigurationDeclaration*()> importConfigDeclCB;
    std::function<ImportedObject*(const QByteArray&, const ConfigurationData&)> loadCB;
};

const ImportedObjectEntryType ImportedObjectList[] = {
    {
        QStringLiteral("Plain Text"),
        ImportedObject::mayOpenAsThisObjectType,
        PlainTextObject::getImportConfigurationDeclaration,
        PlainTextObject::open
    }
};

}

ImportFileDialog::ImportFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportFileDialog),
    openTypes(sizeof(ImportedObjectList)/sizeof(ImportedObjectEntryType))
{
    ui->setupUi(this);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImportFileDialog::updateCurrentInputWidget);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ImportFileDialog::tryOpen);
}

ImportFileDialog::~ImportFileDialog()
{
    delete ui;
}

void ImportFileDialog::setSrc(const QByteArray &srcBA)
{
    src = srcBA;

    int cnt = 0;
    for(int i = 0, n = openTypes.size(); i < n; ++i) {
        const ImportedObjectEntryType& choice = ImportedObjectList[i];
        auto& entry = openTypes[i];
        if (choice.contentCheckCB(srcBA, entry.importConfig)) {
            entry.isApplicable = true;
            ui->comboBox->addItem(choice.fileTypeDisplayName, QVariant(i));
            cnt += 1;
        }
    }
    if (cnt == 0)
        return;
    ui->comboBox->setCurrentIndex(0);
}

void ImportFileDialog::updateCurrentInputWidget() {
    int index = ui->comboBox->currentData().toInt();
    Q_ASSERT(index >= 0 && index < openTypes.size());
    auto& entry = openTypes[index];
    Q_ASSERT(entry.isApplicable);
    if (entry.inputWidget == nullptr) {
        const ConfigurationDeclaration* decl = ImportedObjectList[index].importConfigDeclCB();
        if (decl && (decl->getNumFields() > 0)) {
            ConfigurationInputWidget* inputWidget = new ConfigurationInputWidget;
            inputWidget->setConfigurationDeclaration(decl, entry.importConfig);
            entry.inputWidget = inputWidget;
            ui->placeholderStackedWidget->addWidget(inputWidget);
        } else {
            entry.inputWidget = ui->dummyPage;
        }
    }
    ui->placeholderStackedWidget->setCurrentWidget(entry.inputWidget);
}

void ImportFileDialog::tryOpen()
{
    int index = ui->comboBox->currentData().toInt();
    if (index == -1) {
        accept();
        return;
    }
    Q_ASSERT(index >= 0 && index < openTypes.size());
    auto& entry = openTypes[index];
    Q_ASSERT(entry.isApplicable);
    ConfigurationData importConfig;
    if (ConfigurationInputWidget* inputWidget = qobject_cast<ConfigurationInputWidget*>(entry.inputWidget)) {
        if (!inputWidget->getConfigurationData(importConfig)) {
            QMessageBox::critical(this,
                                  tr("Invalid configuration input"),
                                  tr("The current import setting is invalid. Please fix them first."));
            return;
        }
    }
    if (ImportedObject* obj = ImportedObjectList[index].loadCB(src, importConfig)) {
        result = obj;
        accept();
    } else {
        QMessageBox::critical(this,
                              tr("Open failed"),
                              tr("Failed to import using current configuration."));
        return;
    }
}

