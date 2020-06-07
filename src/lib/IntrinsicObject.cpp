#include "IntrinsicObject.h"
#include "src/lib/DataObject/GeneralTreeObject.h"
#include "src/lib/TaskObject/TestTaskObject.h"
#include "src/lib/TaskObject/SimpleTreeTransformObject.h"
#include "src/lib/TaskObject/SimpleParserObject.h"
#include "src/lib/TaskObject/SimpleTextGeneratorObject.h"
#include "src/lib/TaskObject/SimpleWorkflowObject.h"
#include "src/utils/XMLUtilities.h"
#ifndef PP_DISABLE_GUI
#include "src/gui/SimpleTextGenerator/SimpleTextGeneratorGUIObject.h"
#include "src/gui/SimpleParser/SimpleParserGUIObject.h"
#endif
#include <QDebug>
#include <QSaveFile>

IntrinsicObject::IntrinsicObject(ObjectType ty)
    : FileBackedObject(ty)
{

}

namespace {
const QString XML_TOP = QStringLiteral("PrepPipeIntrinsicObject");
const QString XML_TYPE = QStringLiteral("Type");
} // end of anonymous namespace

void IntrinsicObject::saveToXML(QXmlStreamWriter& xml)
{
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);
    xml.writeStartDocument();
    QString comment = getComment();
    if (!comment.isEmpty()) {
        xml.writeComment(comment);
    }
    xml.writeStartElement(XML_TOP);
    xml.writeAttribute(XML_TYPE, getTypeClassName());
    saveToXMLImpl(xml);
    xml.writeEndElement();
    xml.writeEndDocument();
}

IntrinsicObject* IntrinsicObject::loadFromXML(QXmlStreamReader& xml)
{
    StringCache strCache;
    const char* curElement = "IntrinsicObject";
    QString comment;
    while (xml.readNext() != QXmlStreamReader::StartElement) {
        if (xml.tokenType() == QXmlStreamReader::Comment) {
            if (!comment.isEmpty()) {
                comment.append('\n');
            }
            comment.append(xml.text());
        } else if (xml.tokenType() != QXmlStreamReader::Characters && xml.tokenType() != QXmlStreamReader::StartDocument) {
            auto msg = qWarning();
            XMLError::errorCommon(msg, xml, curElement);
            msg << "Unexpected xml token " << xml.tokenString();
            return nullptr;
        }
    }
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);

    if (Q_UNLIKELY(xml.name() != XML_TOP)) {
        auto msg = qWarning();
        XMLError::errorCommon(msg, xml, curElement);
        msg << "Unexpected xml top level element \"" << xml.name() << "\" (expecting " << XML_TOP << ")";
        return nullptr;
    }


    int objTyEnum = -1;
    {
        auto readObjectType = [&](QStringRef str) -> bool {
            bool isGood = false;
            objTyEnum = QMetaEnum::fromType<ObjectBase::ObjectType>().keyToValue(str.toLatin1().constData(), &isGood);
            return isGood;
        };

        if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_TYPE, readObjectType, {}, "Unrecognized object type"))) {
            return nullptr;
        }
    }

    IntrinsicObject* obj = nullptr;

    switch (static_cast<ObjectBase::ObjectType>(objTyEnum)) {
    default: qFatal("Unhandled Intrinsic Object load; the file is likely being created by newer versions of the program. Please update this program first."); break;
    case ObjectType::Data_GeneralTree:
        obj = IntrinsicObjectTrait<GeneralTreeObject>::loadFromXML(xml, strCache);
        break;
    case ObjectType::Task_Test:
        obj = IntrinsicObjectTrait<TestTaskObject>::loadFromXML(xml, strCache);
        break;
    case ObjectType::Task_SimpleTreeTransform:
        obj = IntrinsicObjectTrait<SimpleTreeTransformObject>::loadFromXML(xml, strCache);
        break;
    case ObjectType::Task_SimpleParser:
        obj = IntrinsicObjectTrait<SimpleParserObject>::loadFromXML(xml, strCache);
        break;
    case ObjectType::Task_SimpleTextGenerator:
        obj = IntrinsicObjectTrait<SimpleTextGeneratorObject>::loadFromXML(xml, strCache);
        break;
    case ObjectType::Task_SimpleWorkflow:
        obj = IntrinsicObjectTrait<SimpleWorkflowObject>::loadFromXML(xml, strCache);
        break;
    }
    if (obj) {
        obj->setComment(comment);
    }

    return obj;
}

bool IntrinsicObject::saveToFile()
{
    QString path = getFilePath();
    QSaveFile f(path);
    f.open(QIODevice::WriteOnly);
    {
        QXmlStreamWriter xml(&f);
        IntrinsicObject::saveToXML(xml);
    }
    return f.commit();
}

QWidget* IntrinsicObject::getEditor()
{
    TextEditor* ui = new TextEditor;
    Q_ASSERT(ui);
    QString text;
    QXmlStreamWriter xml(&text);
    IntrinsicObject::saveToXML(xml);
    ui->setPlainText(text);
    ui->setReadOnly(true);
    return ui;
}

// ----------------------------------------------------------------------------

std::vector<const IntrinsicObjectDecl*>* IntrinsicObjectDecl::instancePtrVec = nullptr;

const std::vector<const IntrinsicObjectDecl*>& IntrinsicObjectDecl::getInstancePtrVec()
{
    Q_ASSERT(instancePtrVec);
    return *instancePtrVec;
}

IntrinsicObjectDecl::IntrinsicObjectDecl()
{
    if (!instancePtrVec) {
        instancePtrVec = new std::vector<const IntrinsicObjectDecl*>;
    }
    instancePtrVec->push_back(this);
}

