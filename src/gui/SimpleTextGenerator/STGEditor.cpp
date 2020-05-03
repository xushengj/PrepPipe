#include "STGEditor.h"
#include "ui_STGEditor.h"
#include <QMessageBox>
#include <QInputDialog>

#include "src/utils/NameSorting.h"

STGEditor::STGEditor(QWidget *parent) :
    EditorBase(parent),
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
    connect(ui->aliasListWidget, &STGAliasListWidget::dirty, this, &STGEditor::setDataDirty);
    connect(ui->headerFragmentInputWidget, &STGFragmentInputWidget::dirty, this, &STGEditor::setDataDirty);
    connect(ui->delimiterFragmentInputWidget, &STGFragmentInputWidget::dirty, this, &STGEditor::setDataDirty);
    connect(ui->tailFragmentInputWidget, &STGFragmentInputWidget::dirty, this, &STGEditor::setDataDirty);

    ui->canonicalNameListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->canonicalNameListWidget, &QWidget::customContextMenuRequested, this, &STGEditor::canonicalNameListContextMenuRequested);

    // TODO we have not yet implemented the generation settings dialog
    ui->generalSettingsPushButton->setEnabled(false);
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
        NameSorting::sortNameList(canonicalNameList);
        ui->canonicalNameListWidget->addItems(canonicalNameList);
        tryGoToRule(0);
    }
}

void STGEditor::setDataDirty()
{
    if (isDuringRuleSwitch)
        return;
    setDirty();
}

void STGEditor::canonicalNameListContextMenuRequested(const QPoint& pos)
{
    QListWidgetItem* item = ui->canonicalNameListWidget->itemAt(pos);

    QMenu menu(ui->canonicalNameListWidget);
    QString oldName;
    if (item) {
        oldName = item->text();
        QAction* renameAction = new QAction(tr("Rename"));
        connect(renameAction, &QAction::triggered, this, [=](){
            QString newName = getCanonicalNameEditDialog(oldName, false);
            if (newName.isEmpty() || newName == oldName)
                return;
            // if the name is already in use, pop up a dialog
            int existingIndex = canonicalNameList.indexOf(newName);
            if (existingIndex != -1) {
                QMessageBox::warning(this, tr("Rename failed"), tr("There is already a rule named \"%1\".").arg(newName));
                tryGoToRule(existingIndex);
                return;
            }
            // switch to empty one first
            QString currentCanonicalName;
            if (currentIndex != -1) {
                currentCanonicalName = canonicalNameList.at(currentIndex);
                tryGoToRule(-1);
            }
            if (currentCanonicalName == oldName) {
                currentCanonicalName = newName;
            }
            // now do the renaming
            {
                auto iter = allData.find(oldName);
                Q_ASSERT(iter != allData.end());

                // make a copy
                auto data = iter.value();

                allData.erase(iter);
                allData.insert(newName, data);

            }
            canonicalNameList.removeOne(oldName);
            canonicalNameList.push_back(newName);
            NameSorting::sortNameList(canonicalNameList);
            refreshCanonicalNameListWidget();
            // switch back
            if (!currentCanonicalName.isEmpty()) {
                int index = canonicalNameList.indexOf(currentCanonicalName);
                Q_ASSERT(index != -1);
                tryGoToRule(index);
            }
            setDataDirty();
        });
        menu.addAction(renameAction);

        QAction* deleteAction = new QAction(tr("Delete"));
        connect(deleteAction, &QAction::triggered, this, [=](){
            if (QMessageBox::question(this,
                                      tr("Delete rule confirmation"),
                                      tr("Are you sure you want to delete the rule \"%1\"?").arg(oldName),
                                      QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes) {
                bool isOneOpen = false;
                if (currentIndex != -1) {
                    isOneOpen = true;
                    if (canonicalNameList.at(currentIndex) == oldName) {
                        tryGoToRule(-1);
                    }
                }
                allData.remove(oldName);
                canonicalNameList.removeOne(oldName);
                refreshCanonicalNameListWidget();
                if (isOneOpen && !canonicalNameList.isEmpty()) {
                    tryGoToRule(0);
                }
                setDataDirty();
            }
        });
        menu.addAction(deleteAction);
    }
    QAction* newAction = new QAction(tr("New"));
    connect(newAction, &QAction::triggered, this, [=](){
        QString newName = getCanonicalNameEditDialog(oldName, true);
        if (newName.isEmpty() || newName == oldName)
            return;
        // if the name is already in use, pop up a dialog
        int existingIndex = canonicalNameList.indexOf(newName);
        if (existingIndex != -1) {
            QMessageBox::warning(this, tr("Rename failed"), tr("There is already a rule named \"%1\".").arg(newName));
            tryGoToRule(existingIndex);
            return;
        }
        tryGoToRule(-1);
        canonicalNameList.push_back(newName);
        NameSorting::sortNameList(canonicalNameList);
        refreshCanonicalNameListWidget();
        int index = canonicalNameList.indexOf(newName);
        allData.insert(newName, RuleData());
        tryGoToRule(index);
        setDataDirty();
    });
    menu.addAction(newAction);
    menu.exec(ui->canonicalNameListWidget->mapToGlobal(pos));
}

QString STGEditor::getCanonicalNameEditDialog(const QString& oldName, bool isNewInsteadofRename)
{
    QString title;
    QString prompt;
    if (isNewInsteadofRename) {
        title = tr("New rule");
        prompt = tr("Please input the name of new rule:");
    } else {
        title = tr("Rename rule");
        prompt = tr("Please input the new name for rule\"%1\":").arg(oldName);
    }
    bool ok = false;
    QString result = QInputDialog::getText(this, title, prompt, QLineEdit::Normal, oldName, &ok);
    if (ok) {
        return result;
    }
    return QString();
}

void STGEditor::refreshCanonicalNameListWidget()
{
    ui->canonicalNameListWidget->clear();
    ui->canonicalNameListWidget->addItems(canonicalNameList);
}

void STGEditor::saveToObjectRequested(ObjectBase* obj)
{
    SimpleTextGeneratorGUIObject* backObj = qobject_cast<SimpleTextGeneratorGUIObject*>(obj);
    Q_ASSERT(backObj == backedObj);

    int curIndex = currentIndex;
    // copy back all data
    tryGoToRule(-1);
    SimpleTextGenerator::Data data;
    backObj->clearGUIData();
    data.unknownNodePolicy = (errorOnUnknownNode? decltype (data.unknownNodePolicy)::Error : decltype (data.unknownNodePolicy)::Ignore);
    data.evalFailPolicy = (errorOnEvaluationFail? decltype (data.evalFailPolicy)::Error : decltype (data.evalFailPolicy)::SkipSubExpr);
    for (auto iter = allData.begin(), iterEnd = allData.end(); iter != iterEnd; ++iter) {
        const QString& cname = iter.key();
        const RuleData& rData = iter.value();
        SimpleTextGenerator::NodeExpansionRule& dest = data.expansions[cname];
        QStringList list;
        dest.header = STGFragmentInputWidget::getDataToStorage(list, rData.header);
        backObj->setHeaderExampleText(cname, list);
        list.clear();
        dest.delimiter = STGFragmentInputWidget::getDataToStorage(list, rData.delimiter);
        backObj->setDelimiterExampleText(cname, list);
        list.clear();
        dest.tail = STGFragmentInputWidget::getDataToStorage(list, rData.tail);
        backObj->setTailExampleText(cname, list);
        if (!rData.aliasList.isEmpty()) {
            data.nameAliases.insert(cname, rData.aliasList);
        }
    }
    backObj->setData(data);

    // switch back
    tryGoToRule(curIndex);

    clearDirty();
}

void STGEditor::tryGoToRule(int index)
{
    isDuringRuleSwitch =  true;
    if (currentIndex != -1) {
        // we are leaving a page; save what we have now
        Q_ASSERT(!canonicalNameList.isEmpty());
        auto& data = getData(currentIndex);
        data.isAllFieldsEnabled = ui->dtEnableCheckBox->isChecked();
        data.aliasList = ui->aliasListWidget->getData();
        ui->headerFragmentInputWidget->getData(data.header);
        ui->delimiterFragmentInputWidget->getData(data.delimiter);
        ui->tailFragmentInputWidget->getData(data.tail);
    }
    if (index == -1) {
        ui->aliasListWidget->setUnbacked();
        ui->headerFragmentInputWidget->setUnbacked();
        ui->delimiterFragmentInputWidget->setUnbacked();
        ui->tailFragmentInputWidget->setUnbacked();
        ui->dtEnableCheckBox->setChecked(false);
        ui->dtEnableCheckBox->setEnabled(false);
        ui->delimiterFragmentInputWidget->setHidden(true);
        ui->tailFragmentInputWidget->setHidden(true);
    } else {
        auto& data = getData(index);
        ui->aliasListWidget->setCanonicalName(canonicalNameList.at(index));
        ui->aliasListWidget->setData(data.aliasList);
        ui->dtEnableCheckBox->setEnabled(true);
        ui->dtEnableCheckBox->setChecked(data.isAllFieldsEnabled);
        ui->delimiterFragmentInputWidget->setHidden(!data.isAllFieldsEnabled);
        ui->tailFragmentInputWidget->setHidden(!data.isAllFieldsEnabled);
        ui->headerFragmentInputWidget->setData(data.header);
        ui->delimiterFragmentInputWidget->setData(data.delimiter);
        ui->tailFragmentInputWidget->setData(data.tail);
    }
    currentIndex = index;
    isDuringRuleSwitch = false;
}

void STGEditor::allFieldCheckboxToggled(bool checked)
{
    Q_UNUSED(checked)
    ui->delimiterFragmentInputWidget->setHidden(!ui->dtEnableCheckBox->isChecked());
    ui->tailFragmentInputWidget->setHidden(!ui->dtEnableCheckBox->isChecked());
}


