#ifndef SIMPLEWORKFLOWOBJECT_H
#define SIMPLEWORKFLOWOBJECT_H

#include "src/lib/TaskObject.h"

#include <QObject>
#include <QString>
#include <QHash>
#include <QVector>

struct SimpleWorkflow {
public:
    // nested type declaration
    enum class ChildObjectInputOption {
        ExportAsAnonymousInput,
        HardwiredNameReference,
        InternalInput
    };
    enum class ChildObjectOutputOption {
        ExportAsOutput,
        IntermediateOutput
    };

    struct ChildObjectInputDecision {
        ChildObjectInputOption option;
        QString inputName; // inputName if exported as anonymous input, name for named reference if hardwired

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };
    struct ChildObjectOutputDecision {
        ChildObjectOutputOption option;
        QString outputName;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct Job {
        QString name; // a descriptive (GUI oriented) name for the job
        QString referenceName; // the name of task object when forming named reference
        ConfigurationData config;
        QHash<QString, ChildObjectInputDecision> inputSpecification;
        QHash<QString, ChildObjectOutputDecision> outputSpecification;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

public:
    // data members
    QVector<Job> jobs;

public:
    // functions
    void saveToXML(QXmlStreamWriter& xml) const;
    bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
};

class SimpleWorkflowExecuteObject : public ExecuteObject
{
    Q_OBJECT

public:
    struct DataFeedDestination {
        QString inputName;
        int destinationIndex = -1; // index in childExecuteObjects
    };

public:
    SimpleWorkflowExecuteObject(const SimpleWorkflow& dataArg, QString name,
                                QList<ExecuteObject*> childArg,
                                QHash<QString, QVector<DataFeedDestination>> inputHandling,
                                QHash<QString, QVector<DataFeedDestination>> outputHandling);
    virtual ~SimpleWorkflowExecuteObject() override {}

    virtual void setInput(QString inputName, ObjectBase* obj) override;

    virtual void getChildExecuteObjects(QList<ExecuteObject*>& list) override {
        list = childExecuteObjects;
    }

protected:
    virtual int startImpl(ExitCause& cause) override;

private:
    void broadcastObject(ObjectBase* obj, const QVector<DataFeedDestination>& dest);

private:
    SimpleWorkflow data;
    QList<ExecuteObject*> childExecuteObjects;
    QHash<QString, QVector<DataFeedDestination>> inputHandlingData;
    QHash<QString, QVector<DataFeedDestination>> outputHandlingData;
};

class SimpleWorkflowObject : public TaskObject
{
    Q_OBJECT

public:
    explicit SimpleWorkflowObject(const SimpleWorkflow& dataArg);
    SimpleWorkflowObject();
    SimpleWorkflowObject(const SimpleWorkflowObject& src) = default;
    virtual ~SimpleWorkflowObject() override {}

    virtual SimpleWorkflowObject* clone() override {
        return new SimpleWorkflowObject(*this);
    }

    static SimpleWorkflowObject* loadFromXML(QXmlStreamReader& xml, StringCache &strCache);

    virtual PreparationError getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const override;

    virtual SimpleWorkflowExecuteObject* getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const override;

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

private:
    SimpleWorkflow data;
};



#endif // SIMPLEWORKFLOWOBJECT_H
