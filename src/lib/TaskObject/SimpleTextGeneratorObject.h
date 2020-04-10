#ifndef SIMPLETEXTGENERATOROBJECT_H
#define SIMPLETEXTGENERATOROBJECT_H

#include "src/lib/TaskObject.h"
#include "src/lib/ExecuteObject.h"
#include "src/lib/Tree/SimpleTextGenerator.h"

#include <QObject>

class SimpleTextGeneratorExecuteObject : public ExecuteObject
{
    Q_OBJECT
public:
    SimpleTextGeneratorExecuteObject(const SimpleTextGenerator::Data& dataArg, QString name);
    virtual ~SimpleTextGeneratorExecuteObject() override {}

    virtual void setInput(QString inputName, ObjectBase* obj) override;

protected:
    virtual int startImpl(ExitCause& cause) override;

private:
    SimpleTextGenerator generator;
    Tree src;
};

class SimpleTextGeneratorObject : public TaskObject
{
    Q_OBJECT
public:
    explicit SimpleTextGeneratorObject(const SimpleTextGenerator::Data& dataArg);
    SimpleTextGeneratorObject();
    SimpleTextGeneratorObject(const SimpleTextGeneratorObject& src) = default;
    virtual ~SimpleTextGeneratorObject() override {}

    virtual SimpleTextGeneratorObject* clone() override {
        return new SimpleTextGeneratorObject(*this);
    }

    static SimpleTextGeneratorObject* loadFromXML(QXmlStreamReader& xml, StringCache &strCache);

    virtual PreparationError getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const override;

    virtual SimpleTextGeneratorExecuteObject* getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const override;

    const SimpleTextGenerator::Data& getData() const {return data;}

    void setData(const SimpleTextGenerator::Data& dataArg) {
        data = dataArg;
    }

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

protected:
    SimpleTextGenerator::Data data;
};

#endif // SIMPLETEXTGENERATOROBJECT_H
