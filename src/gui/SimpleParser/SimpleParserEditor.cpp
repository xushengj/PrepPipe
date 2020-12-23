#include "SimpleParserEditor.h"
#include "ui_SimpleParserEditor.h"

SimpleParserEditor::SimpleParserEditor(QWidget *parent) :
    EditorBase(parent),
    ui(new Ui::SimpleParserEditor)
{
    ui->setupUi(this);

    ruleCtl.init(ui->ruleNameTreeView, ui->ruleStackedWidget, &graphData);
    ruleCtl.setGetNameCallback([](const SimpleParser::MatchRuleNode& data) -> QString {
        return data.name;
    });
    ruleCtl.setCreateWidgetCallback(std::bind(&SimpleParserEditor::createRuleInputWidget, this, std::placeholders::_1));
    connect(ruleCtl.getObj(), &HierarchicalElementTreeControllerObject::dirty, this, &EditorBase::setDirty);

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


SPRuleInputWidget* SimpleParserEditor::createRuleInputWidget(HierarchicalElementTreeControllerObject *obj)
{
    SPRuleInputWidget* w = new SPRuleInputWidget;
    w->setParentNodeNameCheckCallback   (std::bind(&SimpleParserEditor::inputValidationCallback_MatchRuleNode,  this, std::placeholders::_1));
    w->setNamedBoundaryCheckCallback    (std::bind(&SimpleParserEditor::inputValidationCallback_NamedBoundary,  this, std::placeholders::_1));
    w->setContentTypeCheckCallback      (std::bind(&SimpleParserEditor::inputValidationCallback_ContentType,    this, std::placeholders::_1));
    w->bindCommonHelper(ruleCommonHelper);
    connect(w, &SPRuleInputWidget::gotoRuleNodeRequested, obj, &HierarchicalElementTreeControllerObject::tryGoToElement);
    connect(w, &SPRuleInputWidget::dirty, this, &EditorBase::setDirty);
    connect(markCtl.getObj(),       &NamedElementListControllerObject::listUpdated, w, &SPRuleInputWidget::otherDataUpdated);
    connect(contentCtl.getObj(),    &NamedElementListControllerObject::listUpdated, w, &SPRuleInputWidget::otherDataUpdated);
    connect(this,                   &SimpleParserEditor::initComplete,              w, &SPRuleInputWidget::otherDataUpdated);
    return w;
}

SPMarkInputWidget* SimpleParserEditor::createMarkInputWidget(NamedElementListControllerObject* obj)
{
    SPMarkInputWidget* w = new SPMarkInputWidget;
    w->setNamedBoundaryCheckCallback(std::bind(&SimpleParserEditor::inputValidationCallback_NamedBoundary,  this, std::placeholders::_1));
    w->setNamedBoundaryListModel(markCtl.getNameListModel());
    connect(w, &SPMarkInputWidget::gotoMarkRequested, obj, &NamedElementListControllerObject::tryGoToElement);
    connect(w, &SPMarkInputWidget::dirty, this, &EditorBase::setDirty);
    return w;
}

SPContentInputWidget* SimpleParserEditor::createContentInputWidget(NamedElementListControllerObject* obj)
{
    // TODO
    // currently basically a stub
    Q_UNUSED(obj)
    SPContentInputWidget* w = new SPContentInputWidget;
    connect(w, &SPContentInputWidget::dirty, this, &EditorBase::setDirty);
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

    // TODO add settings and others
    auto data = castedObj->getData();
    ruleCtl.getData(data.matchRuleNodes);
    contentCtl.getData(data.contentTypes);
    markCtl.getData(data.namedBoundaries);
    graphData.getData(data); // save after ruleCtl so that rules are already there
    castedObj->setData(data);
}

void SimpleParserEditor::setBackingObject(SimpleParserGUIObject* obj)
{
    backingObj = obj;
    const auto& data = obj->getData();

    graphData.setData(data);
    ruleCtl.setData(data.matchRuleNodes);
    contentCtl.setData(data.contentTypes);
    markCtl.setData(data.namedBoundaries);

    emit initComplete();
    // TODO get rule common helper from file
}

// ---------------------------------------------------------

void SimpleParserEditor::RuleNodeGraphData::setData(const SimpleParser::Data& data)
{
    topNodeList = data.topNodeList;
    childList.clear();
    for (const auto& rule : data.matchRuleNodes) {
        childList.insert(rule.name, rule.childNodeNameList);
    }
}

void SimpleParserEditor::RuleNodeGraphData::getData(SimpleParser::Data& data)
{
    // all rules should already be set
    data.topNodeList = topNodeList;
    for (auto& rule : data.matchRuleNodes) {
        auto iter = childList.find(rule.name);
        if (iter == childList.end() || iter.value().isEmpty()) {
            rule.childNodeNameList.clear();
        } else {
            rule.childNodeNameList = iter.value();
        }
    }
}

void SimpleParserEditor::RuleNodeGraphData::renameElement(const QString& oldName, const QString& newName)
{
    // replace top first
    int topIndex = topNodeList.indexOf(oldName);
    if (topIndex >= 0) {
        topNodeList.replace(topIndex, newName);
    }

    // then the outgoing edges
    {
        auto iter = childList.find(oldName);
        if (iter != childList.end()) {
            auto edges = iter.value();
            childList.erase(iter);
            childList.insert(newName, edges);
        }
    }

    // then incoming edges
    {
        for (auto iter = childList.begin(), iterEnd = childList.end(); iter != iterEnd; ++iter) {
            auto& list = iter.value();
            int index = list.indexOf(oldName);
            if (index >= 0) {
                list.replace(index, newName);
            }
        }
    }
}

void SimpleParserEditor::RuleNodeGraphData::removeAllEdgeWith(const QString& name)
{
    topNodeList.removeAll(name);
    childList.remove(name);
    for (auto iter = childList.begin(), iterEnd = childList.end(); iter != iterEnd; ++iter) {
        iter.value().removeAll(name);
    }
}

void SimpleParserEditor::RuleNodeGraphData::notifyNewElementAdded(const QString& name)
{
    bool isElementAlreadyInUse = topNodeList.contains(name);
    if (!isElementAlreadyInUse) {
        for (auto iter = childList.begin(), iterEnd = childList.end(); iter != iterEnd; ++iter) {
            if (iter.key() == name || iter.value().contains(name)) {
                isElementAlreadyInUse = true;
                break;
            }
        }
    }
    if (!isElementAlreadyInUse) {
        topNodeList.push_back(name);
    }
}


