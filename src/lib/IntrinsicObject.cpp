#include "IntrinsicObject.h"
#include "src/lib/DataObject/GeneralTreeObject.h"
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
    xml.writeStartElement(XML_TOP);
    xml.writeAttribute(XML_TYPE, getTypeClassName());
    xml.writeAttribute(XML_NAME, getName());
    xml.writeTextElement(XML_COMMENT, getComment());
    saveToXMLImpl(xml);
    xml.writeEndElement();
    xml.writeEndDocument();
}

IntrinsicObject* IntrinsicObject::loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        QDebug& dbg = qDebug().noquote();
        dbg << "Read first StartElement failed";
        if (xml.hasError()) {
            dbg << " (" << xml.errorString() << ")";
        }
        dbg << " (Token = " << xml.tokenString() << ")";
        return nullptr;
    }
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);

    if (Q_UNLIKELY(xml.name() != XML_TOP)) {
        qWarning() << "Unexpected xml top level element" << xml.name() << "(expecting " << XML_TOP << ")";
        return nullptr;
    }

    ConstructOptions newOpt = opt;
    int objTyEnum = -1;
    {
        auto attr = xml.attributes();
        // get object type
        if (Q_UNLIKELY(!attr.hasAttribute(XML_TYPE))) {
            qWarning() << "Missing " << XML_TYPE << " attribute on element " << XML_TOP;
            return nullptr;
        }
        QStringRef tyNameRef = attr.value(XML_TYPE);
        QByteArray objTypeName = tyNameRef.toLatin1();
        bool isGood = false;
        objTyEnum = QMetaEnum::fromType<ObjectBase::ObjectType>().keyToValue(objTypeName.constData(), &isGood);
        if (!isGood) {
            qWarning() << "Unexpected object type " << tyNameRef;
            return nullptr;
        }

        // get object name
        if (Q_UNLIKELY(!attr.hasAttribute(XML_NAME))) {
            qWarning() << "Missing " << XML_NAME << " attribute on element " << XML_TOP;
            return nullptr;
        }
        QString nameFromXML = attr.value(XML_NAME).toString();

        if (newOpt.name.isEmpty()) {
            newOpt.name = nameFromXML;
        } else {
            // if the object name do not match with the file name, here we transparently rename the object
            // (by not saving the name from xml)
            if (newOpt.name != nameFromXML) {
                qWarning() << "Intrinsic Object from " << newOpt.filePath << " is renamed from " << nameFromXML << " to " << newOpt.name;
            }
        }
    }

    // get comment
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "Missing " << XML_COMMENT << "element under "<< XML_TOP;
        return nullptr;
    }
    newOpt.comment = xml.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);

    // seek to next element, where data will start
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "Missing data after " << XML_COMMENT << "element under "<< XML_TOP;
        return nullptr;
    }

    IntrinsicObject* obj = nullptr;

    switch (static_cast<ObjectBase::ObjectType>(objTyEnum)) {
    default: qFatal("Unhandled Intrinsic Object load"); break;
    case ObjectType::GeneralTreeObject:
        obj = GeneralTreeObject::loadFromXML(xml, newOpt);
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
