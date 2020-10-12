#ifndef SIMPLEPARSERGUIEXECUTEOBJECT_H
#define SIMPLEPARSERGUIEXECUTEOBJECT_H

#include <QObject>
#include <QWidget>

#include "src/lib/TaskObject/SimpleParserObject.h"
#include "src/gui/TransformPassView/TransformPassViewWidget.h"
#include "src/gui/GeneralTreeEditor.h"
#include "src/gui/TextEditor.h"

class SimpleParserGUIExecuteObject : public SimpleParserExecuteObject
{
    Q_OBJECT
public:
    SimpleParserGUIExecuteObject(const SimpleParser::Data& dataArg, QString name);

    virtual ~SimpleParserGUIExecuteObject() = default;

    virtual QWidget* getViewer() override;

    const QString& getInputText() const {return text;}
    const Tree& getOutputTree() const {return treeOut;}

signals:
    void guiDataReady(EventLogger* logger);

protected:
    virtual int startImpl(ExitCause& cause) override;
};

class SimpleParserViewDelegateObject: public TransformPassViewDelegateObject
{
public:
    SimpleParserViewDelegateObject(SimpleParserGUIExecuteObject* p);
    virtual ~SimpleParserViewDelegateObject() = default;

public:
    virtual QWidget* getDataWidget(TransformPassViewWidget* w, int objectID) override;

    virtual void updateDataWidgetForNewData() override;

private:
    SimpleParserGUIExecuteObject* parent;
    TextEditor* inputViewer;
    GeneralTreeEditor* outputViewer;
};

#endif // SIMPLEPARSERGUIEXECUTEOBJECT_H
