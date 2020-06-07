#include "IntrinsicObjectCreationDialog.h"
#include "ui_IntrinsicObjectCreationDialog.h"

#include <QMessageBox>

IntrinsicObjectCreationDialog::IntrinsicObjectCreationDialog(const ObjectContext &ctx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IntrinsicObjectCreationDialog),
    mainCtx(ctx)
{
    ui->setupUi(this);

    buildIntrinsicObjectList();
    ui->typeTreeWidget->setHeaderHidden(true);
    connect(ui->typeTreeWidget, &QTreeWidget::itemActivated, this, &IntrinsicObjectCreationDialog::switchToType);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &IntrinsicObjectCreationDialog::tryAccept);
}

void IntrinsicObjectCreationDialog::buildIntrinsicObjectList()
{
    std::map<int, std::size_t> sortOrderToIndexMap;
    auto& vec = IntrinsicObjectDecl::getInstancePtrVec();
    for (std::size_t i = 0, n = vec.size(); i < n; ++i) {
        int order = static_cast<int>(vec.at(i)->getObjectType());
        sortOrderToIndexMap.insert(std::make_pair(order, i));
    }

    int numRows = static_cast<int>(sortOrderToIndexMap.size());
    sortedIntrinsicObjectDeclVec.reserve(numRows);
    objectItems.reserve(numRows);
    for (auto& p : sortOrderToIndexMap) {
        const auto* ptr = vec.at(p.second);
        sortedIntrinsicObjectDeclVec.push_back(ptr);

        ObjectBase::ObjectType ty = static_cast<ObjectBase::ObjectType>(p.first);
        QTreeWidgetItem* item = new QTreeWidgetItem;
        item->setText(0, ObjectBase::getTypeDisplayName(ty));
        item->setIcon(0, ObjectBase::getTypeDisplayIcon(ty));
        objectItems.push_back(item);
    }

    // establish parent-child relationship
    for (int curIndex = objectItems.size()-1; curIndex >= 0; --curIndex) {
        const auto* ptr = sortedIntrinsicObjectDeclVec.at(curIndex);
        auto childMax = ptr->getChildMax();

        // skip leaf items
        if (childMax == ptr->getObjectType()) {
            continue;
        }

        int ub = static_cast<int>(childMax);
        QTreeWidgetItem* item = objectItems.at(curIndex);
        for (int childIndex = curIndex+1, n = objectItems.size(); childIndex < n; ++childIndex) {
            const auto* childPtr = sortedIntrinsicObjectDeclVec.at(childIndex);
            if (static_cast<int>(childPtr->getObjectType()) >= ub) {
                break;
            }
            QTreeWidgetItem* childItem = objectItems.at(childIndex);

            // skip items that already have immediate parents
            if (childItem->parent()) {
                continue;
            }

            item->addChild(childItem);
            Q_ASSERT(childItem->parent() != nullptr);
        }
    }

    ui->typeTreeWidget->clear();
    for (int i = 0, n = objectItems.size(); i < n; ++i) {
        QTreeWidgetItem* item = objectItems.at(i);
        itemToIndexMap.insert(item, i);
        if (!item->parent()) {
            ui->typeTreeWidget->addTopLevelItem(item);
            item->setExpanded(true);
        }
    }
}

IntrinsicObjectCreationDialog::~IntrinsicObjectCreationDialog()
{
    delete ui;
}

void IntrinsicObjectCreationDialog::switchToType(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (item) {
        int index = itemToIndexMap.value(item, -1);
        Q_ASSERT(index >= 0);
        ui->documentBrowser->setHtml(sortedIntrinsicObjectDeclVec.at(index)->getHTMLDocumentation());
    } else {
        ui->documentBrowser->clear();
    }
}

void IntrinsicObjectCreationDialog::tryAccept()
{
    // check if we have selected a valid type
    const IntrinsicObjectDecl* declPtr = nullptr;
    if (QTreeWidgetItem* item = ui->typeTreeWidget->currentItem()) {
        int curIndex = itemToIndexMap.value(item, -1);
        Q_ASSERT(curIndex >= 0);
        auto* ptr = sortedIntrinsicObjectDeclVec.at(curIndex);
        if (ptr->getChildMax() == ptr->getObjectType()) {
            declPtr = ptr;
        } else {
            QMessageBox::warning(this, tr("Invalid type selection"), tr("Please select a concrete object type instead of a category."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Invalid type selection"), tr("Please select the intended object type from the list."));
        return;
    }

    // check if we have a valid name
    QString objName = ui->nameLineEdit->text().trimmed();
    if (objName.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid name"), tr("New object name cannot be empty."));
        return;
    }
    if (mainCtx.getObject(objName)) {
        QMessageBox::warning(this, tr("Invalid name"), tr("There is already an object with the same name."));
        return;
    }

    IntrinsicObject* obj = declPtr->create(objName);
    Q_ASSERT(obj);

    // make sure we can save this object
    obj->setFilePath(mainCtx.getDirectory() + "/" + objName + ".xml");
    if (obj->saveToFile()) {
        resultObj = obj;
        accept();
    } else {
        // save failed
        QMessageBox::warning(this, tr("Save failed"), tr("Cannot save the new object with the given name. Please check if the name satisfy file name requirement."));
        delete obj;
    }

}
