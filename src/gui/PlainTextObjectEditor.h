#ifndef PLAINTEXTOBJECTEDITOR_H
#define PLAINTEXTOBJECTEDITOR_H

#include <QObject>
#include <QWidget>

#include "src/gui/EditorBase.h"
#include "src/gui/TextEditor.h"

class PlainTextObjectEditor : public EditorBase
{
    Q_OBJECT
public:
    PlainTextObjectEditor(QWidget* parent = nullptr);

    TextEditor* getEditor() {return editor;}

public:
    virtual void saveToObjectRequested(ObjectBase* obj) override;

private:
    TextEditor* editor;
};

#endif // PLAINTEXTOBJECTEDITOR_H
