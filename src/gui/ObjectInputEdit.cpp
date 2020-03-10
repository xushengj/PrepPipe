#include "ObjectInputEdit.h"
#include "ui_ObjectInputEdit.h"

#include "src/gui/ObjectTreeWidget.h"

#include <QDragEnterEvent>
#include <QMimeData>

ObjectInputEdit::ObjectInputEdit(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ObjectInputEdit)
{
    ui->setupUi(this);
    setAcceptDrops(true);
}

ObjectInputEdit::~ObjectInputEdit()
{
    delete ui;
}

void ObjectInputEdit::setInputRequirement(const QVector<ObjectBase::ObjectType>& tys) {
    acceptedTypes = tys;
    resultRef = ObjectContext::AnonymousObjectReference();
    refreshView();
}

void ObjectInputEdit::trySetReference(ObjectContext::AnonymousObjectReference ref)
{
    ObjectBase* obj = ObjectContext::resolveAnonymousReference(ref);
    if (obj && acceptedTypes.contains(obj->getType())) {
        resultRef = ref;
    } else {
        resultRef = ObjectContext::AnonymousObjectReference();
    }
    refreshView();
}

void ObjectInputEdit::dragEnterEvent(QDragEnterEvent* event)
{
    isInDragDrop = true;
    bool isDropGood = true;
    QList<ObjectContext::AnonymousObjectReference> newRefList = ObjectTreeWidget::recoverReference(event->mimeData());
    if (newRefList.size() != 1) {
        ui->label->setText(tr("1 object expected instead of %1").arg(QString::number(newRefList.size())));
        isDropGood = false;
    } else {
        ObjectBase* obj = ObjectContext::resolveAnonymousReference(newRefList.front());
        if (Q_UNLIKELY(!obj)) {
            ui->label->setText(tr("Invalid object reference"));
            isDropGood = false;
        } else if (Q_UNLIKELY(!acceptedTypes.contains(obj->getType()))) {
            ui->label->setText(tr("Invalid type %1").arg(obj->getTypeDisplayName()));
            isDropGood = false;
        }

    }

    if (isDropGood)
        ui->label->setText(tr("Drop here for input"));

    setBackgroundRole(QPalette::AlternateBase);
    event->acceptProposedAction();
}

void ObjectInputEdit::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
    isInDragDrop = false;
    refreshView();
}

void ObjectInputEdit::dropEvent(QDropEvent* event)
{
    bool isDropGood = false;
    QList<ObjectContext::AnonymousObjectReference> newRefList = ObjectTreeWidget::recoverReference(event->mimeData());
    if (newRefList.size() == 1) {
        ObjectBase* obj = ObjectContext::resolveAnonymousReference(newRefList.front());
        if (obj && acceptedTypes.contains(obj->getType())) {
            resultRef = newRefList.front();
            isDropGood = true;
        }
    }
    if (isDropGood) {
        event->accept();
    } else {
        event->ignore();
    }
    refreshView();
}

void ObjectInputEdit::refreshView()
{
    setBackgroundRole(QPalette::Base);
    ObjectBase* obj = resultRef.isValid()? ObjectContext::resolveAnonymousReference(resultRef): nullptr;
    if (obj && acceptedTypes.contains(obj->getType())) {
        ui->label->setPixmap(obj->getTypeDisplayIcon().pixmap(ui->label->height(), ui->label->height()));
        ui->label->setText(obj->getName());
    } else {
        if (resultRef.isValid()) {
            // drop the reference if it is invalid now
            resultRef = ObjectContext::AnonymousObjectReference();
        }
        ui->label->setPixmap(QPixmap());
        ui->label->setText(tr("<No selection>"));
    }
}
