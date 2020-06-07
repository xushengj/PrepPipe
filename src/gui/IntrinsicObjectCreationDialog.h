#ifndef INTRINSICOBJECTCREATIONDIALOG_H
#define INTRINSICOBJECTCREATIONDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "src/lib/ObjectContext.h"
#include "src/lib/IntrinsicObject.h"

namespace Ui {
class IntrinsicObjectCreationDialog;
}

class IntrinsicObjectCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IntrinsicObjectCreationDialog(const ObjectContext& ctx, QWidget *parent = nullptr);
    ~IntrinsicObjectCreationDialog();

    IntrinsicObject* getObject() const {
        return resultObj;
    }

private:
    void buildIntrinsicObjectList();

private slots:
    void switchToType(QTreeWidgetItem* item, int column);
    void tryAccept();

private:
    Ui::IntrinsicObjectCreationDialog *ui;
    const ObjectContext& mainCtx;
    IntrinsicObject* resultObj = nullptr;

    QVector<QTreeWidgetItem*> objectItems;
    QVector<const IntrinsicObjectDecl*> sortedIntrinsicObjectDeclVec;
    QHash<QTreeWidgetItem*, int> itemToIndexMap;
};

#endif // INTRINSICOBJECTCREATIONDIALOG_H
