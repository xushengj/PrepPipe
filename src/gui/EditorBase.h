#ifndef EDITORBASE_H
#define EDITORBASE_H

#include "src/lib/ObjectBase.h"

#include <QWidget>

class EditorBase : public QWidget
{
    Q_OBJECT
public:
    explicit EditorBase(QWidget *parent = nullptr);
    virtual ~EditorBase(){}

signals:
    void dirty();

public:
    virtual void saveToObjectRequested(ObjectBase* obj) {Q_UNUSED(obj) clearDirty();}

    bool isDirty() {return dirtyFlag;}

public slots:
    void setDirty();
    void clearDirty() {
        dirtyFlag = false;
    }

protected:
    bool dirtyFlag = false;
};

#endif // EDITORBASE_H
