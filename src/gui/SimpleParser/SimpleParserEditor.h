#ifndef SIMPLEPARSEREDITOR_H
#define SIMPLEPARSEREDITOR_H

#include "src/gui/EditorBase.h"
#include "src/gui/NamedElementListController.h"
#include "src/gui/SimpleParser/SimpleParserGUIObject.h"
#include "src/gui/SimpleParser/SPRuleInputWidget.h"
#include "src/gui/SimpleParser/SPMarkInputWidget.h"
#include "src/gui/SimpleParser/SPContentInputWidget.h"

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

signals:
    // private signal; should not be used outside
    void initComplete();

private:
    SPRuleInputWidget* createRuleInputWidget(NamedElementListControllerObject* obj);
    SPMarkInputWidget* createMarkInputWidget(NamedElementListControllerObject* obj);
    SPContentInputWidget* createContentInputWidget(NamedElementListControllerObject* obj);
    bool inputValidationCallback_MatchRuleNode(const QString& name);
    bool inputValidationCallback_ContentType(const QString& name);
    bool inputValidationCallback_NamedBoundary(const QString& name);

private:
    Ui::SimpleParserEditor *ui;
    SimpleParserGUIObject* backingObj = nullptr;
    NamedElementListController<SPRuleInputWidget, true> ruleCtl;
    NamedElementListController<SPMarkInputWidget, true> markCtl;
    NamedElementListController<SPContentInputWidget, true> contentCtl;
    SPRuleInputWidget::CommonHelperData ruleCommonHelper;
};

#endif // SIMPLEPARSEREDITOR_H
