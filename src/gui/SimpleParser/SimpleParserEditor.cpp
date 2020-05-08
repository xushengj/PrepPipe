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

    markCtl.init(ui->markNameListWidget, ui->markStackedWidget);
    markCtl.setGetNameCallback([](const SimpleParser::NamedBoundary& data) -> QString {
        return data.name;
    });
    markCtl.setCreateWidgetCallback(std::bind(&SimpleParserEditor::createMarkInputWidget, this, std::placeholders::_1));
    connect(markCtl.getObj(), &NamedElementListControllerObject::dirty, this, &EditorBase::setDirty);

    contentCtl.init(ui->contentNameListWidget, ui->contentStackedWidget);
    contentCtl.setGetNameCallback([](const SimpleParser::ContentType& data) -> QString {
        return data.name;
    });
    contentCtl.setCreateWidgetCallback(std::bind(&SimpleParserEditor::createContentInputWidget, this, std::placeholders::_1));
    connect(contentCtl.getObj(), &NamedElementListControllerObject::dirty, this, &EditorBase::setDirty);
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

SPMarkInputWidget* SimpleParserEditor::createMarkInputWidget(NamedElementListControllerObject* obj)
{
    SPMarkInputWidget* w = new SPMarkInputWidget;
    w->setNamedBoundaryCheckCallback(std::bind(&SimpleParserEditor::inputValidationCallback_NamedBoundary,  this, std::placeholders::_1));
    w->setNamedBoundaryListModel(markCtl.getNameListModel());
    connect(w, &SPMarkInputWidget::gotoMarkRequested, obj, &NamedElementListControllerObject::tryGoToElement);
    return w;
}

SPContentInputWidget* SimpleParserEditor::createContentInputWidget(NamedElementListControllerObject* obj)
{
    // TODO
    // currently basically a stub
    Q_UNUSED(obj)
    SPContentInputWidget* w = new SPContentInputWidget;
    return w;
}

bool SimpleParserEditor::inputValidationCallback_MatchRuleNode(const QString& name)
{
    return ruleCtl.isElementExist(name);
}

bool SimpleParserEditor::inputValidationCallback_ContentType(const QString& name)
{
    return contentCtl.isElementExist(name);
}

bool SimpleParserEditor::inputValidationCallback_NamedBoundary(const QString& name)
{
    return markCtl.isElementExist(name);
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
    const auto& data = obj->getData();
    ruleCtl.setData(data.matchRuleNodes);
    contentCtl.setData(data.contentTypes);
    markCtl.setData(data.namedBoundaries);
}
