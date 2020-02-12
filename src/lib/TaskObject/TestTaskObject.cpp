#include "TestTaskObject.h"

#include <QDebug>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

TestTaskObject::TestTaskObject(const TestExecuteObject::TestSettings &settingsArg, const ConstructOptions &options)
    : TaskObject(ObjectType::Task_Test, options), settings(settingsArg)
{

}

TestTaskObject::TestTaskObject(const ConstructOptions &options)
    : TaskObject(ObjectType::Task_Test, options), settings()
{

}

TestTaskObject::~TestTaskObject()
{

}

TaskObject::PreparationError TestTaskObject::getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const
{
    Q_UNUSED(config)
    Q_UNUSED(in)
    Q_UNUSED(out)
    Q_UNUSED(resolveReferenceCB)
    return PreparationError();
}

class TestExecuteObject* TestTaskObject::getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const
{
    Q_UNUSED(options)
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)
    return new class TestExecuteObject(settings, getName());
}

namespace {
const QString STR_SLEEP_DURATION = QStringLiteral("Sleep");
const QString STR_CAUSE = QStringLiteral("ExitCause");
} // end of anonymous namespace

void TestTaskObject::saveToXMLImpl(QXmlStreamWriter& xml)
{
    xml.writeTextElement(STR_SLEEP_DURATION, QString::number(settings.sleepDuration));
    xml.writeTextElement(STR_CAUSE, QString::number(settings.exitCause));
}

TestTaskObject* TestTaskObject::loadFromXML(QXmlStreamReader &xml, const ConstructOptions& opt)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    if (Q_UNLIKELY(xml.name() != STR_SLEEP_DURATION)) {
        qWarning() << "Unexpected element " << xml.name() << "(expecting " << STR_SLEEP_DURATION <<")";
        return nullptr;
    }
    TestExecuteObject::TestSettings settings;
    bool isGood = false;
    QString durationStr = xml.readElementText();
    settings.sleepDuration = durationStr.toInt(&isGood);
    if (Q_UNLIKELY(!isGood)) {
        qWarning() << "Invalid" << STR_SLEEP_DURATION << ":" << durationStr;
        return nullptr;
    }

    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "Missing " << STR_CAUSE << " element";
        return nullptr;
    }
    if (Q_UNLIKELY(xml.name() != STR_CAUSE)) {
        qWarning() << "Unexpected element " << xml.name() << "(expecting " << STR_CAUSE <<")";
        return nullptr;
    }

    QString causeStr = xml.readElementText();
    settings.exitCause = causeStr.toInt(&isGood);
    if (Q_UNLIKELY(!isGood)) {
        qWarning() << "Invalid" << STR_CAUSE << ":" << causeStr;
        return nullptr;
    }

    return new TestTaskObject(settings, opt);
}

//-----------------------------------------------------------------------------

TestExecuteObject::TestExecuteObject(const TestSettings& test, const QString &name)
    : ExecuteObject(ObjectType::Exec_Test, ConstructOptions(name, QString())), settings(test)
{}

TestExecuteObject::~TestExecuteObject() {}

int TestExecuteObject::startImpl(ExitCause& cause)
{
    for (int i = 0; i < settings.sleepDuration; ++i) {
        emit statusUpdate("sleeping..", 0, settings.sleepDuration, i);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (isTerminationRequested(cause)) {
            return -1;
        }
    }
    switch(settings.exitCause){
    default: return 0;
    case -1: qFatal("Bad value"); return 0;
    case -2: return *static_cast<volatile int*>(nullptr);
    }
}
