#ifndef SIMPLEPARSEROBJECT_H
#define SIMPLEPARSEROBJECT_H

#include "src/lib/TaskObject.h"
#include "src/lib/ExecuteObject.h"
#include "src/lib/Tree/SimpleParser.h"

#include <QObject>

class SimpleParserExecuteObject : public ExecuteObject
{
    Q_OBJECT
public:
    SimpleParserExecuteObject(const SimpleParser::Data& dataArg, QString name);
    virtual ~SimpleParserExecuteObject() override {}

    virtual void setInput(QString inputName, ObjectBase* obj) override;

protected:
    virtual int startImpl(ExitCause& cause) override;

private:
    SimpleParser parser;
    QString text;
};

class SimpleParserObject : public TaskObject
{
    Q_OBJECT
public:
    explicit SimpleParserObject(const SimpleParser::Data& data);
    SimpleParserObject();
    SimpleParserObject(const SimpleParserObject&) = default;
    virtual ~SimpleParserObject() override {}

    virtual SimpleParserObject* clone() override {
        return new SimpleParserObject(*this);
    }

    static SimpleParserObject* loadFromXML(QXmlStreamReader& xml, StringCache &strCache);

    virtual PreparationError getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const override;

    virtual SimpleParserExecuteObject* getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const override;

    const SimpleParser::Data& getData() const {
        return data;
    }

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

protected:
    SimpleParser::Data data;
};

#endif // SIMPLEPARSEROBJECT_H
