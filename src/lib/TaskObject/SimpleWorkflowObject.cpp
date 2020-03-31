#include "SimpleWorkflowObject.h"

#include <QEventLoop>

SimpleWorkflowObject::SimpleWorkflowObject()
    : TaskObject(ObjectType::Task_SimpleWorkflow)

{

}

SimpleWorkflowObject::SimpleWorkflowObject(const SimpleWorkflow& dataArg)
    : TaskObject(ObjectType::Task_SimpleWorkflow), data(dataArg)
{

}

SimpleWorkflowObject* SimpleWorkflowObject::loadFromXML(QXmlStreamReader& xml, StringCache &strCache)
{
    SimpleWorkflow data;
    if (Q_LIKELY(data.loadFromXML(xml, strCache))) {
        return new SimpleWorkflowObject(data);
    }
    return nullptr;
}

void SimpleWorkflowObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    data.saveToXML(xml);
}

TaskObject::PreparationError SimpleWorkflowObject::getInputOutputInfo(
        const ConfigurationData& config,
        QList<TaskInput>& in,
        QList<TaskOutput>& out,
        std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const
{
    Q_UNUSED(config)

    // step 0: make sure that there is no circular dependencies, which will result in stack overflow
    // TODO

    // do a validation and list requirements at the same time
    struct ChildOutputData {
        int jobIndex = -1;
        ObjectType ty = ObjectType::Data_GeneralTree;
        bool batchOutput = false;
        bool isExport = false;
    };
    struct ChildInputData {
        QVector<ObjectType> acceptedTypes;
        bool acceptBatch = false;
        bool optionalInput = false;
    };
    ChildOutputData mainOutputRecord;
    QHash<QString, ChildOutputData> namedOutputRecord;
    QHash<QString, ChildInputData> inputRecord;
    for (int jobIndex = 0, n = data.jobs.size(); jobIndex < n; ++jobIndex) {
        const auto& job = data.jobs.at(jobIndex);
        const ObjectBase* obj = resolveReferenceCB(ObjectBase::NamedReference(job.referenceName, QStringList()));
        if (Q_UNLIKELY(!obj)) {
            PreparationError err;
            err.cause = PreparationError::Cause::UnresolvedReference;
            err.firstFatalErrorDescription = tr("Job %1: Task object \"%2\" not found!").arg(job.name, job.referenceName);
            err.firstUnresolvedReference = job.referenceName;
            return err;
        }
        const TaskObject* task = qobject_cast<const TaskObject*>(obj);
        if (Q_UNLIKELY(!task)) {
            return PreparationError(tr("Job %1: Object \"%2\" is not a runnable task!").arg(job.name, job.referenceName));
        }
        if (auto* decl = task->getConfigurationDeclaration()) {
            if (Q_UNLIKELY(!job.config.isValid(*decl))) {
                return PreparationError(tr("Job %1: Invalid configuration for Task object \"%2\"").arg(job.name, job.referenceName));
            }
        }
        QList<TaskInput> childInList;
        QList<TaskOutput> childOutList;
        PreparationError childErr = task->getInputOutputInfo(job.config, childInList, childOutList, resolveReferenceCB);
        if (childErr) {
            childErr.context.push_back(tr("Job %1: Error in child task %2").arg(job.name, job.referenceName));
            return childErr;
        }
        // resolve inputs
        QSet<QString> appearedNames;
        for (const auto& childIn : childInList) {
            Q_ASSERT(!appearedNames.contains(childIn.inputName));
            appearedNames.insert(childIn.inputName);
            auto iterSpec = job.inputSpecification.find(childIn.inputName);
            if (iterSpec == job.inputSpecification.end()) {
                if (childIn.flags & InputFlag::InputIsOptional) {
                    continue;
                }
                return PreparationError(tr("Job %1: Unspecified input %2").arg(job.name, getInputDisplayName(childIn.inputName)));
            }
            const auto& decision = iterSpec.value();
            switch (decision.option) {
            case SimpleWorkflow::ChildObjectInputOption::ExportAsAnonymousInput: {
                // create a dummy entry in in first; will fix them later on
                {
                    TaskInput dummyIn;
                    dummyIn.inputName = decision.inputName;
                    in.push_back(dummyIn);
                }

                auto iter = inputRecord.find(decision.inputName);
                if (iter == inputRecord.end()) {
                    // create an input record
                    ChildInputData inData;
                    inData.acceptedTypes = childIn.acceptedType;
                    Q_ASSERT(!inData.acceptedTypes.isEmpty());
                    inData.acceptBatch = childIn.flags & InputFlag::AcceptBatch;
                    inData.optionalInput = childIn.flags & InputFlag::InputIsOptional;
                    inputRecord.insert(decision.inputName, inData);
                } else {
                    // join the requirement of two inputs
                    ChildInputData& curData = iter.value();
                    ChildInputData inData;
                    for (auto ty : childIn.acceptedType) {
                        if (curData.acceptedTypes.contains(ty)) {
                            inData.acceptedTypes.push_back(ty);
                        }
                    }
                    if (Q_UNLIKELY(inData.acceptedTypes.isEmpty())) {
                        return PreparationError(tr("Input %1: no solution to satisfy all users").arg(getInputDisplayName(decision.inputName)));
                    }
                    inData.acceptBatch = (childIn.flags & InputFlag::AcceptBatch) && curData.acceptBatch;
                    inData.optionalInput = (childIn.flags & InputFlag::InputIsOptional) && curData.optionalInput;
                    curData = inData;
                }
            }break;
            case SimpleWorkflow::ChildObjectInputOption::HardwiredNameReference: {
                ObjectBase* obj = resolveReferenceCB(NamedReference(decision.inputName, QStringList()));
                if (Q_UNLIKELY(!obj)) {
                    PreparationError err;
                    err.cause = PreparationError::Cause::UnresolvedReference;
                    err.firstFatalErrorDescription = tr("Job %1: Input object \"%2\" is not found. It is specified as input \"%3\" of task object \"%4\".")
                            .arg(job.name, decision.inputName, childIn.inputName, job.referenceName);
                    err.firstUnresolvedReference = decision.inputName;
                    return err;
                }
                // type check
                if (Q_UNLIKELY(!childIn.acceptedType.contains(obj->getType()))) {
                    return PreparationError(tr("Job %1: Input object %2 in type %3 cannot serve for input \"%4\" of task object \"%5\".")
                                            .arg(job.name, decision.inputName, obj->getTypeDisplayName(), childIn.inputName, job.referenceName));
                }
                // nothing else to be done
            }break;
            case SimpleWorkflow::ChildObjectInputOption::InternalInput: {
                ChildOutputData srcOut;
                if (decision.inputName.isEmpty()) {
                    srcOut = mainOutputRecord;
                } else {
                    auto iter = namedOutputRecord.find(decision.inputName);
                    if (iter != namedOutputRecord.end()) {
                        srcOut = iter.value();
                    }
                }

                if (Q_UNLIKELY(srcOut.jobIndex == -1)) {
                    return PreparationError(tr("Job %1: Input \"%2\" is not found for input \"%3\" of task object \"%4\".")
                                            .arg(job.name, decision.inputName, childIn.inputName, job.referenceName));
                }
                ObjectType objTy = srcOut.ty;
                bool isBatchedOutput = srcOut.batchOutput;
                // type check
                if (Q_UNLIKELY(!childIn.acceptedType.contains(objTy))) {
                    return PreparationError(tr("Job %1: Temporary output object %2 in type %3 cannot serve for input \"%4\" of task object \"%5\".")
                                            .arg(job.name, decision.inputName, ObjectBase::getTypeDisplayName(objTy), childIn.inputName, job.referenceName));
                }
                // also report error if the output is a batch but the input do not accept batch
                if (Q_UNLIKELY(isBatchedOutput && !(childIn.flags & InputFlag::AcceptBatch))) {
                    return PreparationError(tr("Job %1: Temporary output %2 is a batch but the input \"%3\" of task \"%4\" do not accept batch.")
                                            .arg(job.name, decision.inputName, childIn.inputName, job.referenceName));
                }
                // nothing else to be done
            }break;
            }
        }
        // also report error if an input specification is not used
        for (auto iter = job.inputSpecification.begin(), iterEnd = job.inputSpecification.end(); iter != iterEnd; ++iter) {
            if (Q_UNLIKELY(!appearedNames.contains(iter.key()))) {
                return PreparationError(tr("Job %1: No input named \"%2\" is found for task object \"%3\"")
                                        .arg(job.name, iter.key(), job.referenceName));
            }
        }
        // done for input
        appearedNames.clear();

        // handling outputs
        for (const auto& childOut : childOutList) {
            Q_ASSERT(!appearedNames.contains(childOut.outputName));
            appearedNames.insert(childOut.outputName);
            auto iterSpec = job.outputSpecification.find(childOut.outputName);
            if (iterSpec == job.outputSpecification.end()) {
                if (Q_UNLIKELY(!(childOut.flags & OutputFlag::TemporaryOutput))) {
                    return PreparationError(tr("Job %1: Output \"%2\" is not specified for task \"%3\"")
                                            .arg(job.name, childOut.outputName, job.referenceName));
                }
                // otherwise it is perfectly okay to discard a temporary output
            } else {
                ChildOutputData curOut;
                curOut.jobIndex = jobIndex;
                curOut.ty = childOut.ty;
                curOut.batchOutput = childOut.flags & OutputFlag::ProduceBatch;
                const auto& decision = iterSpec.value();
                switch (decision.option) {
                case SimpleWorkflow::ChildObjectOutputOption::ExportAsOutput: {
                    curOut.isExport = true;
                    // directly write it to output now
                    {
                        TaskOutput outEntry;
                        outEntry.outputName = decision.outputName;
                        outEntry.ty = curOut.ty;
                        outEntry.flags = OutputFlag::NoOutputFlag;
                        // for now we assume all outputs are needed (no optional output)
                        if (curOut.batchOutput) {
                            outEntry.flags |= OutputFlag::ProduceBatch;
                        }
                        out.push_back(outEntry);
                    }
                }break;
                case SimpleWorkflow::ChildObjectOutputOption::IntermediateOutput: {
                    curOut.isExport = false;
                }break;
                }

                // actual handling
                if (decision.outputName.isEmpty()) {
                    // export as main output
                    if (Q_UNLIKELY(mainOutputRecord.jobIndex != -1)) {
                        return PreparationError(tr("Multiple definition of main output; first defined in job %1 and second in job %2")
                                                .arg(data.jobs.at(mainOutputRecord.jobIndex).name, job.name));
                    }
                    mainOutputRecord = curOut;
                } else {
                    // export as named output
                    auto iter = namedOutputRecord.find(decision.outputName);
                    if (Q_UNLIKELY(iter != namedOutputRecord.end())) {
                        return PreparationError(tr("Multiple definition of output \"%1\"; first defined in job %2 and second in job %3")
                                                .arg(decision.outputName, data.jobs.at(mainOutputRecord.jobIndex).name, job.name));
                    }
                    namedOutputRecord.insert(decision.outputName, curOut);
                }
            }
        }
        // also report error if an input specification is not used
        for (auto iter = job.outputSpecification.begin(), iterEnd = job.outputSpecification.end(); iter != iterEnd; ++iter) {
            if (Q_UNLIKELY(!appearedNames.contains(iter.key()))) {
                return PreparationError(tr("Job %1: No output named \"%2\" is found for task object \"%3\"")
                                        .arg(job.name, iter.key(), job.referenceName));
            }
        }
    }

    // done handling all outputs
    // now finalize the input requirement
    for (auto& curIn : in) {
        auto iter = inputRecord.find(curIn.inputName);
        Q_ASSERT(iter != inputRecord.end());
        const auto& data = iter.value();
        curIn.flags = InputFlag::NoInputFlag;
        if (data.acceptBatch) {
            curIn.flags |= InputFlag::AcceptBatch;
        }
        if (data.optionalInput) {
            curIn.flags |= InputFlag::InputIsOptional;
        }
        curIn.acceptedType = data.acceptedTypes;
    }
    return PreparationError();
}

SimpleWorkflowExecuteObject* SimpleWorkflowObject::getExecuteObject(
        const LaunchOptions& options,
        const ConfigurationData& config,
        std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const
{
    Q_UNUSED(options)
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)
    QList<ExecuteObject*> childExecuteObjects;
    childExecuteObjects.reserve(data.jobs.size());
    QHash<QString, QVector<SimpleWorkflowExecuteObject::DataFeedDestination>> inputHandlingData;
    QHash<QString, QVector<SimpleWorkflowExecuteObject::DataFeedDestination>> outputHandlingData;
    QSet<QString> exportNames;
    for (const auto& job : data.jobs) {
        // add the child execute object
        ObjectBase* obj = resolveReferenceCB(NamedReference(job.referenceName, QStringList()));
        Q_ASSERT(obj);
        TaskObject* task = qobject_cast<TaskObject*>(obj);
        Q_ASSERT(task);
        ExecuteObject* exec = task->getExecuteObject(options, job.config, resolveReferenceCB);
        Q_ASSERT(exec);

        if (!job.name.isEmpty()) {
            exec->setName(job.name);
        }

        int jobIndex = childExecuteObjects.size();
        // satisfy all named reference input and register the others
        for (auto iter = job.inputSpecification.begin(), iterEnd = job.inputSpecification.end(); iter != iterEnd; ++iter) {
            const QString& inputName = iter.key();
            const auto& spec = iter.value();
            switch (spec.option) {
            case SimpleWorkflow::ChildObjectInputOption::ExportAsAnonymousInput: {
                SimpleWorkflowExecuteObject::DataFeedDestination dest;
                dest.inputName = inputName;
                dest.destinationIndex = jobIndex;
                inputHandlingData[spec.inputName].push_back(dest);
            }break;
            case SimpleWorkflow::ChildObjectInputOption::HardwiredNameReference: {
                ObjectBase* obj = resolveReferenceCB(NamedReference(spec.inputName, QStringList()));
                Q_ASSERT(obj);
                exec->setInput(inputName, obj);
            }break;
            case SimpleWorkflow::ChildObjectInputOption::InternalInput: {
                SimpleWorkflowExecuteObject::DataFeedDestination dest;
                dest.inputName = inputName;
                dest.destinationIndex = jobIndex;
                outputHandlingData[spec.inputName].push_back(dest);
            }break;
            }
        }

        childExecuteObjects.push_back(exec);
    }
    return new class SimpleWorkflowExecuteObject(data, getName(), childExecuteObjects, inputHandlingData, outputHandlingData);
}

//-----------------------------------------------------------------------------

SimpleWorkflowExecuteObject::SimpleWorkflowExecuteObject(const SimpleWorkflow& dataArg, QString name,
        QList<ExecuteObject*> childArg,
        QHash<QString, QVector<DataFeedDestination>> inputHandling,
        QHash<QString, QVector<DataFeedDestination>> outputHandling)
    : ExecuteObject(ObjectType::Exec_SimpleWorkflow, name),
      data(dataArg),
      childExecuteObjects(childArg),
      inputHandlingData(inputHandling),
      outputHandlingData(outputHandling)
{

}

void SimpleWorkflowExecuteObject::setInput(QString inputName, ObjectBase* obj)
{
    auto iter = inputHandlingData.find(inputName);
    Q_ASSERT(iter != inputHandlingData.end());
    broadcastObject(obj, iter.value());
}

void SimpleWorkflowExecuteObject::broadcastObject(
        ObjectBase* obj,
        const QVector<DataFeedDestination>& dest)
{
    for (const auto& target : dest) {
        ExecuteObject* exec = childExecuteObjects.at(target.destinationIndex);
        exec->setInput(target.inputName, obj);
    }
}

int SimpleWorkflowExecuteObject::startImpl(ExitCause& cause)
{
    for (int jobIndex = 0, n = childExecuteObjects.size(); jobIndex < n; ++jobIndex) {
        if (isTerminationRequested(cause)) {
            return -1;
        }
        ExecuteObject* exec = childExecuteObjects.at(jobIndex);
        // void statusUpdate(const QString& description, int start, int end, int value);
        emit statusUpdate(QString(), 0, n * 100, jobIndex * 100);
        QEventLoop loop;
        int childStatus = 0;
        ExitCause childExitCause = ExitCause::Completed;
        connect(exec, &ExecuteObject::statusUpdate, this, [=](const QString& description, int start, int end, int value) -> void {
            int rate = (start == end)? 0: ((value - start) * 100 / (end - start));
            emit statusUpdate(description, 0, n * 100, jobIndex * 100 + rate);
        }, Qt::QueuedConnection);
        connect(exec, &ExecuteObject::outputAvailable, this, [=](const QString& outputName, ObjectBase* obj) -> void {
            const auto& job = data.jobs.at(jobIndex);
            auto iterSpec = job.outputSpecification.find(outputName);
            if (iterSpec == job.outputSpecification.end()) {
                // optional output
                obj->deleteLater();
                return;
            }
            const auto& spec = iterSpec.value();
            {
                auto iter = outputHandlingData.find(spec.outputName);
                if (iter != outputHandlingData.end()) {
                    broadcastObject(obj, iter.value());
                }
            }
            switch (spec.option) {
            case SimpleWorkflow::ChildObjectOutputOption::IntermediateOutput: {
                obj->deleteLater();
            }break;
            case SimpleWorkflow::ChildObjectOutputOption::ExportAsOutput: {
                emit outputAvailable(spec.outputName, obj);
            }break;
            }
        }, Qt::QueuedConnection);
        connect(exec, &ExecuteObject::finished, this, [&](int status, int cause) -> void {
            int exitCode = 0;
            if (status != 0 || static_cast<ExitCause>(cause) != ExitCause::Completed) {
                exitCode = 1;
                childStatus = status;
                childExitCause = static_cast<ExitCause>(cause);
            }
            loop.exit(exitCode);
        }, Qt::QueuedConnection);
        QMetaObject::invokeMethod(exec, &ExecuteObject::start, Qt::QueuedConnection);
        int code = loop.exec();
        if (code != 0) {
            cause = childExitCause;
            return childStatus;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------

namespace {
const QString XML_JOB_LIST = QStringLiteral("Jobs");
const QString XML_JOB = QStringLiteral("Job");
const QString XML_NAME = QStringLiteral("Name");
const QString XML_REFERENCE_NAME = QStringLiteral("TaskReferenceName");
const QString XML_CONFIG = QStringLiteral("Config");
const QString XML_INPUT_LIST = QStringLiteral("Inputs");
const QString XML_INPUT = QStringLiteral("Input");
const QString XML_OUTPUT_LIST = QStringLiteral("Outputs");
const QString XML_OUTPUT = QStringLiteral("Output");
const QString XML_EXPORT = QStringLiteral("Export");
const QString XML_INTERNAL = QStringLiteral("Internal");
const QString XML_NAME_REFERENCE = QStringLiteral("NameReference");
const QString XML_SOURCE = QStringLiteral("Source");
const QString XML_OPTION = QStringLiteral("Option");
}

void SimpleWorkflow::saveToXML(QXmlStreamWriter& xml) const
{
    XMLUtil::writeLoadableList(xml, jobs, XML_JOB_LIST, XML_JOB);
}

bool SimpleWorkflow::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleWorkflow";
    return XMLUtil::readLoadableList(xml, curElement, XML_JOB_LIST, XML_JOB, jobs, strCache);
}

void SimpleWorkflow::Job::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeTextElement(XML_NAME, name);
    xml.writeTextElement(XML_REFERENCE_NAME, referenceName);
    xml.writeStartElement(XML_CONFIG);
    config.saveToXML(xml);
    XMLUtil::writeLoadableHash(xml, inputSpecification, XML_INPUT_LIST, XML_INPUT, XML_NAME);
    XMLUtil::writeLoadableHash(xml, outputSpecification, XML_OUTPUT_LIST, XML_OUTPUT, XML_NAME);
    xml.writeEndElement();
}

bool SimpleWorkflow::Job::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleWorkflow::Job";
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_NAME, name, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_REFERENCE_NAME, referenceName, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadable(xml, curElement, XML_CONFIG, config, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableHash(xml, curElement, XML_INPUT_LIST, XML_INPUT, XML_NAME, inputSpecification, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableHash(xml, curElement, XML_OUTPUT_LIST, XML_OUTPUT, XML_NAME, outputSpecification, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleWorkflow::ChildObjectInputDecision::saveToXML(QXmlStreamWriter& xml) const
{
    switch (option) {
    case ChildObjectInputOption::ExportAsAnonymousInput: {
        xml.writeAttribute(XML_SOURCE, XML_EXPORT);
    }break;
    case ChildObjectInputOption::HardwiredNameReference: {
        xml.writeAttribute(XML_SOURCE, XML_NAME_REFERENCE);
    }break;
    case ChildObjectInputOption::InternalInput: {
        xml.writeAttribute(XML_SOURCE, XML_INTERNAL);
    }break;
    }
    xml.writeCharacters(inputName);
    xml.writeEndElement();
}

bool SimpleWorkflow::ChildObjectInputDecision::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    auto getOptionCB = [this](QStringRef text) -> bool {
        if (text == XML_EXPORT) {
            option = ChildObjectInputOption::ExportAsAnonymousInput;
        } else if (text == XML_NAME_REFERENCE) {
            option = ChildObjectInputOption::HardwiredNameReference;
        } else if (text == XML_INTERNAL) {
            option = ChildObjectInputOption::InternalInput;
        } else {
            return false;
        }
        return true;
    };
    const char* curElement = "SimpleWorkflow::ChildObjectInputDecision";
    if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_SOURCE, getOptionCB, {XML_EXPORT, XML_NAME_REFERENCE, XML_INTERNAL}, nullptr))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readRemainingString(xml, curElement, inputName, strCache))) {
        return false;
    }
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement);
    return true;
}

void SimpleWorkflow::ChildObjectOutputDecision::saveToXML(QXmlStreamWriter& xml) const
{
    switch (option) {
    case ChildObjectOutputOption::ExportAsOutput: {
        xml.writeAttribute(XML_OPTION, XML_EXPORT);
    }break;
    case ChildObjectOutputOption::IntermediateOutput: {
        xml.writeAttribute(XML_OPTION, XML_INTERNAL);
    }break;
    }
    xml.writeCharacters(outputName);
    xml.writeEndElement();
}

bool SimpleWorkflow::ChildObjectOutputDecision::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    auto getOptionCB = [this](QStringRef text) -> bool {
        if (text == XML_EXPORT) {
            option = ChildObjectOutputOption::ExportAsOutput;
        } else if (text == XML_INTERNAL) {
            option = ChildObjectOutputOption::IntermediateOutput;
        } else {
            return false;
        }
        return true;
    };
    const char* curElement = "SimpleWorkflow::ChildObjectOutputDecision";
    if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_OPTION, getOptionCB, {XML_EXPORT, XML_INTERNAL}, nullptr))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readRemainingString(xml, curElement, outputName, strCache))) {
        return false;
    }
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement);
    return true;
}
