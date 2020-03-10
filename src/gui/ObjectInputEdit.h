#ifndef OBJECTINPUTEDIT_H
#define OBJECTINPUTEDIT_H

#include <QWidget>

#include "src/lib/ObjectBase.h"
#include "src/lib/ObjectContext.h"

namespace Ui {
class ObjectInputEdit;
}

class ObjectInputEdit : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectInputEdit(QWidget *parent = nullptr);
    ~ObjectInputEdit();

    void setInputRequirement(const QVector<ObjectBase::ObjectType>& tys);
    void trySetReference(ObjectContext::AnonymousObjectReference ref);
    ObjectContext::AnonymousObjectReference getResult() const {return resultRef;}

protected:
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dropEvent(QDropEvent* event) override;

private:
    void refreshView();

private:
    Ui::ObjectInputEdit *ui;

    // options
    QVector<ObjectBase::ObjectType> acceptedTypes;

    // result
    ObjectContext::AnonymousObjectReference resultRef;

    // states
    bool isInDragDrop = false;
};

#endif // OBJECTINPUTEDIT_H
