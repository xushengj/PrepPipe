#include "SimpleParserGUIExecuteObject.h"

SimpleParserViewDelegateObject::SimpleParserViewDelegateObject(SimpleParserGUIExecuteObject *p)
    : parent(p),
      inputViewer(new TextEditor),
      outputViewer(new GeneralTreeEditor)
{
    inputViewer->setReadOnly(true);
    outputViewer->setReadOnly(true);
}

QWidget* SimpleParserViewDelegateObject::getDataWidget(TransformPassViewWidget* w, int objectID)
{
    switch(objectID) {
    default: qFatal("Unexpected objectID"); return nullptr;
    case static_cast<int>(SimpleParserEvent::EventLocationContext::InputData): {
        return inputViewer;
    }break;
    case static_cast<int>(SimpleParserEvent::EventLocationContext::OutputData): {
        return outputViewer;
    }break;
    }
}

void SimpleParserViewDelegateObject::updateDataWidgetForNewData()
{
    inputViewer->setPlainText(parent->getInputText());
    outputViewer->setData(parent->getOutputTree());
}

SimpleParserGUIExecuteObject::SimpleParserGUIExecuteObject(const SimpleParser::Data& dataArg, QString name)
    : SimpleParserExecuteObject(dataArg, name)
{

}

int SimpleParserGUIExecuteObject::startImpl(ExitCause& cause)
{
    if (logger)
        delete logger;

    logger = new EventLogger;
    logger->installInterpreter(SimpleParserEvent::getInterpreter());
    int retVal = SimpleParserExecuteObject::startImpl(cause);
    // note that we emit the signal no matter what's the cause of termination of execution
    emit guiDataReady(logger);
    return retVal;
}

QWidget* SimpleParserGUIExecuteObject::getViewer()
{
    SimpleParserViewDelegateObject* delegate = new SimpleParserViewDelegateObject(this);
    TransformPassViewWidget* w = new TransformPassViewWidget;
    w->installDelegateObject(delegate);
    connect(this, &SimpleParserGUIExecuteObject::guiDataReady, w, &TransformPassViewWidget::dataReady);
    if (logger) {
        // we already completed one run
        emit guiDataReady(logger);
    }
    return w;
}
