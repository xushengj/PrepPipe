#include "STGEditor.h"
#include "ui_STGEditor.h"

STGEditor::STGEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::STGEditor)
{
    ui->setupUi(this);

    ui->headerFragmentInputWidget->setTitle(tr("Header"));
    ui->delimiterFragmentInputWidget->setTitle(tr("Delimiter"));
    ui->tailFragmentInputWidget->setTitle(tr("Tail"));
    ui->delimiterFragmentInputWidget->setHidden(!ui->dtEnableCheckBox->isChecked());
    ui->tailFragmentInputWidget->setHidden(!ui->dtEnableCheckBox->isChecked());

    connect(ui->canonicalNameListWidget, &QListWidget::currentRowChanged, this, &STGEditor::tryGoToRule);
    connect(ui->dtEnableCheckBox, &QCheckBox::toggled, this, &STGEditor::allFieldCheckboxToggled);
}

STGEditor::~STGEditor()
{
    delete ui;
}

void STGEditor::setBackingObject(SimpleTextGeneratorGUIObject* obj)
{
    // this should only be initialized once
    Q_ASSERT(obj != nullptr);
    Q_ASSERT(backedObj == nullptr);

    backedObj = obj;

    // step 1: populate all data
    const auto& data = obj->getData();
    for (auto iter = data.expansions.begin(), iterEnd = data.expansions.end(); iter != iterEnd; ++iter) {
        const QString& canonicalName = iter.key();
        canonicalNameList.push_back(canonicalName);
        RuleData rData;
        {
            auto iter = data.nameAliases.find(canonicalName);
            if (iter != data.nameAliases.end()) {
                rData.aliasList = iter.value();
            }
        }
        rData.header    = STGFragmentInputWidget::getDataFromStorage(iter.value().header,       obj->getHeaderExampleText(canonicalName));
        rData.delimiter = STGFragmentInputWidget::getDataFromStorage(iter.value().delimiter,    obj->getHeaderExampleText(canonicalName));
        rData.tail      = STGFragmentInputWidget::getDataFromStorage(iter.value().tail,         obj->getHeaderExampleText(canonicalName));
        rData.isAllFieldsEnabled = !(iter.value().delimiter.isEmpty() && iter.value().tail.isEmpty());
        allData.insert(canonicalName, rData);
    }

    errorOnUnknownNode = (data.unknownNodePolicy != decltype (data.unknownNodePolicy) ::Ignore);
    errorOnEvaluationFail = (data.evalFailPolicy != decltype (data.evalFailPolicy) ::SkipSubExpr);

    // step 2: initialize gui
    if (!canonicalNameList.isEmpty()) {
        canonicalNameList.sort();
        ui->canonicalNameListWidget->addItems(canonicalNameList);
        tryGoToRule(0);
    }
}

void STGEditor::writeBack(SimpleTextGeneratorGUIObject* obj)
{
    Q_UNUSED(obj)
    // TODO
}

void STGEditor::tryGoToRule(int index)
{
    if (currentIndex != -1) {
        // we are leaving a page; save what we have now
        Q_ASSERT(!canonicalNameList.isEmpty());
        auto& data = getData(currentIndex);
        data.isAllFieldsEnabled = ui->dtEnableCheckBox->isChecked();
        ui->headerFragmentInputWidget->getData(data.header);
        ui->delimiterFragmentInputWidget->getData(data.delimiter);
        ui->tailFragmentInputWidget->getData(data.tail);
    }
    if (index == -1) {
        ui->headerFragmentInputWidget->setUnbacked();
        ui->delimiterFragmentInputWidget->setUnbacked();
        ui->tailFragmentInputWidget->setUnbacked();
        ui->dtEnableCheckBox->setChecked(false);
        ui->dtEnableCheckBox->setEnabled(false);
        ui->delimiterFragmentInputWidget->setHidden(true);
        ui->tailFragmentInputWidget->setHidden(true);
    } else {
        auto& data = getData(index);
        ui->dtEnableCheckBox->setEnabled(true);
        ui->dtEnableCheckBox->setChecked(data.isAllFieldsEnabled);
        ui->delimiterFragmentInputWidget->setHidden(!data.isAllFieldsEnabled);
        ui->tailFragmentInputWidget->setHidden(!data.isAllFieldsEnabled);
        ui->headerFragmentInputWidget->setData(data.header);
        ui->delimiterFragmentInputWidget->setData(data.delimiter);
        ui->tailFragmentInputWidget->setData(data.tail);
    }
    currentIndex = index;
}

void STGEditor::allFieldCheckboxToggled(bool checked)
{
    Q_UNUSED(checked)
    ui->delimiterFragmentInputWidget->setHidden(!ui->dtEnableCheckBox->isChecked());
    ui->tailFragmentInputWidget->setHidden(!ui->dtEnableCheckBox->isChecked());
}


