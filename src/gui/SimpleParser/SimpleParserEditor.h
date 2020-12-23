#ifndef SIMPLEPARSEREDITOR_H
#define SIMPLEPARSEREDITOR_H

#include "src/gui/EditorBase.h"
#include "src/gui/NamedElementListController.h"
#include "src/gui/HierarchicalElementTreeController.h"
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
    struct RuleNodeGraphData : public HierarchicalElementTreeControllerObject::GraphData
    {
    public:
        virtual ~RuleNodeGraphData() override = default;

        // implemented in cpp file
        virtual void renameElement(const QString& oldName, const QString& newName) override;
        virtual void removeAllEdgeWith(const QString& name) override;
        virtual void notifyNewElementAdded(const QString& name) override;

        // -------------------------------------------------
        virtual QStringList getTopElementList() const override {
            return topNodeList;
        }
        virtual QStringList getChildElementList(const QString& parentElementName) const override {
            return childList.value(parentElementName);
        }
        virtual bool isTopElement(const QString& name) const override {
            return topNodeList.contains(name);
        }
        virtual bool hasChild(const QString& parent, const QString& child) const override {
            auto iter = childList.find(parent);
            return (iter != childList.end() && iter.value().contains(child));
        }

        // -------------------------------------------------
        virtual void addEdge(const QString& parent, const QString& newChild) override {
            auto& list = childList[parent];
            if (!list.contains(newChild)) {
                list.push_back(newChild);
            }
        }
        virtual void removeEdge(const QString& parent, const QString& child) override{
            auto iter = childList.find(parent);
            if (iter != childList.end()) {
                iter.value().removeAll(child);
            }
        }
        virtual void addTopElementReference(const QString& name) override {
            if (!topNodeList.contains(name)) {
                topNodeList.push_back(name);
            }
        }
        virtual void removeTopElementReference(const QString& name) override {
            topNodeList.removeAll(name);
        }

        // use default implementation for the following two functions
        // virtual void rewriteEdge(const QString& parent, const QString& oldChild, const QString& newChild) override;
        // virtual void rewriteTopElementReference(const QString& oldName, const QString& newName) override;

    public:
        // invoked from the editor
        void setData(const SimpleParser::Data& data);
        void getData(SimpleParser::Data& data);

    private:
        QStringList topNodeList;
        QHash<QString, QStringList> childList;
    } graphData;

private:
    SPRuleInputWidget* createRuleInputWidget(HierarchicalElementTreeControllerObject* obj);
    SPMarkInputWidget* createMarkInputWidget(NamedElementListControllerObject* obj);
    SPContentInputWidget* createContentInputWidget(NamedElementListControllerObject* obj);
    bool inputValidationCallback_MatchRuleNode(const QString& name);
    bool inputValidationCallback_ContentType(const QString& name);
    bool inputValidationCallback_NamedBoundary(const QString& name);

private:
    Ui::SimpleParserEditor *ui;
    SimpleParserGUIObject* backingObj = nullptr;
    HierarchicalElementTreeController<SPRuleInputWidget, true> ruleCtl;
    NamedElementListController<SPMarkInputWidget, true> markCtl;
    NamedElementListController<SPContentInputWidget, true> contentCtl;
    SPRuleInputWidget::CommonHelperData ruleCommonHelper;
};

#endif // SIMPLEPARSEREDITOR_H
