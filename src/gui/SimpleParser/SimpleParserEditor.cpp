#include "SimpleParserEditor.h"
#include "ui_SimpleParserEditor.h"

SimpleParserEditor::SimpleParserEditor(QWidget *parent) :
    EditorBase(parent),
    ui(new Ui::SimpleParserEditor)
{
    ui->setupUi(this);

    ruleCtl.init(ui->ruleNameListWidget, ui->ruleStackedWidget);
    ruleCtl.setGetNameCallback([](const SimpleParser::MatchRuleNode& data) -> QString {
        return data.name;
    });
    ruleCtl.setCreateWidgetCallback(std::bind(&SimpleParserEditor::createRuleInputWidget, this, std::placeholders::_1));
    connect(ruleCtl.getObj(), &NamedElementListControllerObject::dirty, this, &EditorBase::setDirty);
}

SimpleParserEditor::~SimpleParserEditor()
{
    delete ui;
}


SPRuleInputWidget* SimpleParserEditor::createRuleInputWidget(NamedElementListControllerObject* obj)
{
    SPRuleInputWidget* w = new SPRuleInputWidget;
    w->setParentNodeNameCheckCallback   (std::bind(&SimpleParserEditor::inputValidationCallback_MatchRuleNode,  this, std::placeholders::_1));
    w->setNamedBoundaryCheckCallback    (std::bind(&SimpleParserEditor::inputValidationCallback_NamedBoundary,  this, std::placeholders::_1));
    w->setContentTypeCheckCallback      (std::bind(&SimpleParserEditor::inputValidationCallback_ContentType,    this, std::placeholders::_1));
    connect(w, &SPRuleInputWidget::gotoRuleNodeRequested, obj, &NamedElementListControllerObject::tryGoToElement);
    return w;
}

bool SimpleParserEditor::inputValidationCallback_MatchRuleNode(const QString& name)
{
    return ruleCtl.isElementExist(name);
}

bool SimpleParserEditor::inputValidationCallback_ContentType(const QString& name)
{
    // for now just a stub
    // TODO
    return true;
}

bool SimpleParserEditor::inputValidationCallback_NamedBoundary(const QString& name)
{
    // for now just a stub
    // TODO
    return true;
}

void SimpleParserEditor::saveToObjectRequested(ObjectBase* obj)
{
    SimpleParserGUIObject* castedObj = qobject_cast<SimpleParserGUIObject*>(obj);
    Q_ASSERT(backingObj == castedObj);

    // TODO
}

void SimpleParserEditor::setBackingObject(SimpleParserGUIObject* obj)
{
    backingObj = obj;
    ruleCtl.setData(obj->getData().matchRuleNodes);
}
