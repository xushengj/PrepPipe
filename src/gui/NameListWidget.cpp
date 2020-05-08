#include "NameListWidget.h"

#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

NameListWidget::NameListWidget(QWidget *parent)
    : QListWidget(parent),
      failIcon(":/icon/execute/failed.png")
{

}

void NameListWidget::setData(const QStringList& dataArg)
{
    clear();
    addItems(dataArg);
    data = dataArg;
}

void NameListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem* item = itemAt(event->pos());

    QMenu menu(this);
    if (item) {
        QAction* editAction = new QAction(tr("Edit"));
        connect(editAction, &QAction::triggered, this, [=](){
            editRequested(item);
        });
        menu.addAction(editAction);
        QAction* deleteAction = new QAction(tr("Delete"));
        connect(deleteAction, &QAction::triggered, this, [=](){
            deleteRequested(item);
        });
        menu.addAction(deleteAction);
        if (isAcceptGotoRequest && checkNameCB && checkNameCB(item->text())) {
            QAction* gotoAction = new QAction(tr("Goto"));
            connect(gotoAction, &QAction::triggered, this, [=](){
                emit gotoRequested(item->text());
            });
            menu.addAction(gotoAction);
        }
    }
    QAction* newAction = new QAction(tr("New"));
    connect(newAction, &QAction::triggered, this, &NameListWidget::newRequested);
    menu.addAction(newAction);

    menu.exec(mapToGlobal(event->pos()));
    event->accept();
}

void NameListWidget::newRequested()
{
    QString title;
    if (referredDisplayName.isEmpty()) {
        title = tr("New");
    } else {
        title = tr("New %1").arg(referredDisplayName);
    }
    QString label = tr("Please input the new %1 here:").arg(referredDisplayName.isEmpty()? tr("entry") : referredDisplayName);
    bool isOkay = false;
    QString result = QInputDialog::getText(this, title, label, QLineEdit::Normal, defaultName, &isOkay);
    if (!isOkay || result.isEmpty())
        return;

    int existingIndex = data.indexOf(result);
    if (existingIndex != -1) {
        setCurrentRow(existingIndex);
        return;
    }

    QListWidgetItem* item = new QListWidgetItem(result);
    if (checkNameCB) {
        if (!checkNameCB(result)) {
            item->setIcon(failIcon);
        }
    }
    addItem(item);
    int index = data.size();
    data.push_back(result);
    Q_ASSERT(row(item) == index);
    emit dirty();
}

void NameListWidget::editRequested(QListWidgetItem* item){
    // almost the same code as in new request
    QString title;
    if (referredDisplayName.isEmpty()) {
        title = tr("Edit");
    } else {
        title = tr("Edit %1").arg(referredDisplayName);
    }
    QString label = tr("Please input the new %1 here:").arg(referredDisplayName.isEmpty()? tr("entry") : referredDisplayName);
    QString oldString = item->text();
    int index = data.indexOf(oldString);
    Q_ASSERT(index != -1);
    bool isOkay = false;
    QString result = QInputDialog::getText(this, title, label, QLineEdit::Normal, oldString, &isOkay);
    if (!isOkay || result.isEmpty() || result == oldString)
        return;

    int existingIndex = data.indexOf(result);
    if (existingIndex != -1) {
        setCurrentRow(existingIndex);
        return;
    }

    data[index] = result;
    item->setText(result);
    if (checkNameCB) {
        if (!checkNameCB(result)) {
            item->setIcon(failIcon);
        } else {
            item->setIcon(QIcon());
        }
    }
    emit dirty();
}

void NameListWidget::deleteRequested(QListWidgetItem* item)
{
    int index = row(item);
    data.removeAt(index);
    delete item;
    emit dirty();
}

void NameListWidget::nameCheckUpdateRequested()
{
    for (int i = 0, n = data.size(); i < n; ++i) {
        QListWidgetItem* itemPtr = item(i);
        bool isFail = false;
        if (checkNameCB) {
            if (!checkNameCB(itemPtr->text())) {
                isFail = true;
            }
        }
        if (isFail) {
            itemPtr->setIcon(failIcon);
        } else {
            itemPtr->setIcon(QIcon());
        }
    }
}
