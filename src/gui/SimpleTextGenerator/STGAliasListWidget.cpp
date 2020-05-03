#include "STGAliasListWidget.h"
#include "src/utils/NameSorting.h"

#include <QMenu>
#include <QAction>

STGAliasListWidget::STGAliasListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setSortingEnabled(true);
    connect(this, &QListWidget::itemDoubleClicked, this, &STGAliasListWidget::editAliasRequested);
    setUnbacked();
}

void STGAliasListWidget::setUnbacked()
{
    clear();
    setEnabled(false);
}

void STGAliasListWidget::setData(const QStringList& alias)
{
    originalData = alias;
    clear();
    setEnabled(true);
    for (const QString& name : alias) {
        addAlias(name);
    }
    sortItems();
    isDirty = false;
}

QListWidgetItem* STGAliasListWidget::addAlias(const QString& str)
{
    QListWidgetItem* item = new QListWidgetItem(str);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
    addItem(item);
    return item;
}

void STGAliasListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem* item = itemAt(event->pos());
    QMenu menu(this);
    if (item) {
        QAction* editAliasAction = new QAction(tr("Edit"));
        connect(editAliasAction, &QAction::triggered, this, [=](){
            editAliasRequested(item);
        });
        menu.addAction(editAliasAction);
        QAction* deleteAliasAction = new QAction(tr("Delete"));
        connect(deleteAliasAction, &QAction::triggered, this, [=](){
            deleteAliasRequested(item);
        });
        menu.addAction(deleteAliasAction);
    }
    QAction* newAliasAction = new QAction(tr("New"));
    QString newDefaultName = cname;
    if (item) {
        newDefaultName = item->text();
    }
    connect(newAliasAction, &QAction::triggered, this, [=](){
        newAliasRequested(newDefaultName);
    });
    menu.addAction(newAliasAction);

    menu.exec(mapToGlobal(event->pos()));
}

void STGAliasListWidget::newAliasRequested(const QString& initialName)
{
    QListWidgetItem* item = addAlias(initialName);
    addItem(item);
    editItem(item);
    flagDirty();
}

void STGAliasListWidget::editAliasRequested(QListWidgetItem* item)
{
    editItem(item);
    flagDirty();
}

void STGAliasListWidget::deleteAliasRequested(QListWidgetItem* item)
{
    delete item;
    flagDirty();
}

QStringList STGAliasListWidget::getData()
{
    if (!isDirty)
        return originalData;

    QStringList newData;
    for (int i = 0, n = count(); i < n; ++i) {
        newData.push_back(item(i)->text());
        NameSorting::sortNameList(newData);
        newData.removeDuplicates();
    }
    return newData;
}
