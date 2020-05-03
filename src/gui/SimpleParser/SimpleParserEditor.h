#ifndef SIMPLEPARSEREDITOR_H
#define SIMPLEPARSEREDITOR_H

#include "src/gui/EditorBase.h"
#include "src/gui/NamedElementListController.h"
#include "src/gui/SimpleParser/SimpleParserGUIObject.h"
#include "src/gui/SimpleParser/SPRuleInputWidget.h"

#include <QWidget>

namespace Ui {
class SimpleParserEditor;
}

class SimpleParserEditor : public EditorBase
{
    Q_OBJECT

public:
    explicit SimpleParserEditor(QWidget *parent = nullptr);
    ~SimpleParserEditor();

    // functions that would be called from gui object
    void setBackingObject(SimpleParserGUIObject* obj);

    virtual void saveToObjectRequested(ObjectBase* obj) override;

private:
    SPRuleInputWidget* createRuleInputWidget(NamedElementListControllerObject* obj);
    bool inputValidationCallback_MatchRuleNode(const QString& name);
    bool inputValidationCallback_ContentType(const QString& name);
    bool inputValidationCallback_NamedBoundary(const QString& name);

private:
    Ui::SimpleParserEditor *ui;
    SimpleParserGUIObject* backingObj = nullptr;
    NamedElementListController<SPRuleInputWidget, true> ruleCtl;
};

#endif // SIMPLEPARSEREDITOR_H
