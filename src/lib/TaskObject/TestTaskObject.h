#ifndef TESTTASKOBJECT_H
#define TESTTASKOBJECT_H

#include "src/lib/TaskObject.h"
#include "src/lib/ExecuteObject.h"

#include <QObject>

class TestExecuteObject : public ExecuteObject
{
    Q_OBJECT
public:
    struct TestSettings {
        int sleepDuration = 1;
        int exitCause = 0; // 0: normal; 1: qFatal(); 2: segfault
    };
    TestExecuteObject(const TestSettings& test, const QString& name);
    virtual ~TestExecuteObject() override;

protected:
    virtual int startImpl(ExitCause& cause) override;

private:
    const TestSettings settings;
};

class TestTaskObject : public TaskObject
{
    Q_OBJECT
public:
    TestTaskObject(const ConstructOptions& options);
    TestTaskObject(const TestExecuteObject::TestSettings& settingsArg ,const ConstructOptions& options);
    TestTaskObject(const TestTaskObject& src) = default;

    virtual TestTaskObject* clone() override {
        return new TestTaskObject(*this);
    }

    virtual ~TestTaskObject() override;

    virtual PreparationError getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const override;

    virtual class TestExecuteObject* getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const override;

    static TestTaskObject* loadFromXML(QXmlStreamReader &xml, const ConstructOptions& opt);

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter& xml) override;

private:
    TestExecuteObject::TestSettings settings;
};

#endif // TESTTASKOBJECT_H
