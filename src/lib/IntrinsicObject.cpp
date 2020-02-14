#include "IntrinsicObject.h"
#include "src/lib/DataObject/GeneralTreeObject.h"
#include "src/lib/TaskObject/TestTaskObject.h"
#include "src/lib/TaskObject/SimpleTreeTransformObject.h"
#include "src/utils/XMLUtilities.h"
#include <QDebug>

IntrinsicObject::IntrinsicObject(ObjectType ty, const ConstructOptions &opt)
    : ObjectBase(ty, opt)
{

}

namespace {
const QString XML_TOP = QStringLiteral("SUPPIntrinsicObject");
const QString XML_TYPE = QStringLiteral("Type");
const QString XML_NAME = QStringLiteral("Name");
const QString XML_COMMENT = QStringLiteral("Comment");
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
    xml.writeAttribute(XML_NAME, getName());
    saveToXMLImpl(xml);
    xml.writeEndElement();
    xml.writeEndDocument();
}

IntrinsicObject* IntrinsicObject::loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt)
{
    StringCache strCache;
    ConstructOptions newOpt = opt;
    const char* curElement = "IntrinsicObject";
    while (xml.readNext() != QXmlStreamReader::StartElement) {
        if (xml.tokenType() == QXmlStreamReader::Comment) {
            if (!newOpt.comment.isEmpty()) {
                newOpt.comment.append('\n');
            }
            newOpt.comment.append(xml.text());
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
    QString nameFromXML;
    {
        auto readObjectType = [&](QStringRef str) -> bool {
            bool isGood = false;
            objTyEnum = QMetaEnum::fromType<ObjectBase::ObjectType>().keyToValue(str.toLatin1().constData(), &isGood);
            return isGood;
        };

        if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_TYPE, readObjectType, {}, "Unrecognized object type"))) {
            return nullptr;
        }
        if (Q_UNLIKELY(!XMLUtil::readStringAttribute(xml, curElement, QString(), XML_NAME, nameFromXML, strCache))) {
            return nullptr;
        }
    }

    if (newOpt.name.isEmpty()) {
        newOpt.name = nameFromXML;
    } else {
        // if the object name do not match with the file name, here we transparently rename the object
        // (by not saving the name from xml)
        if (newOpt.name != nameFromXML) {
            qWarning() << "Intrinsic Object from " << newOpt.filePath << " is renamed from " << nameFromXML << " to " << newOpt.name;
        }
    }

    IntrinsicObject* obj = nullptr;

    switch (static_cast<ObjectBase::ObjectType>(objTyEnum)) {
    default: qFatal("Unhandled Intrinsic Object load"); break;
    case ObjectType::Data_GeneralTree:
        obj = GeneralTreeObject::loadFromXML(xml, newOpt, strCache);
        break;
    case ObjectType::Task_Test:
        obj = TestTaskObject::loadFromXML(xml, newOpt);
        break;
    case ObjectType::Task_SimpleTreeTransform:
        obj = SimpleTreeTransformObject::loadFromXML(xml, newOpt, strCache);
        break;
    }

    return obj;
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
