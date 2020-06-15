#include "GeneralTreeObject.h"
#include "src/utils/XMLUtilities.h"
#include "src/gui/GeneralTreeEditor.h"

#include <QDebug>

GeneralTreeObject::GeneralTreeObject()
    : IntrinsicObject(ObjectType::Data_GeneralTree)
{

}

GeneralTreeObject::GeneralTreeObject(const Tree &tree)
    : IntrinsicObject(ObjectType::Data_GeneralTree), treeData(tree)
{

}

GeneralTreeObject::~GeneralTreeObject()
{

}

QWidget* GeneralTreeObject::getEditor()
{
    return getViewer();
}

QWidget* GeneralTreeObject::getViewer()
{
    GeneralTreeEditor* viewer = new GeneralTreeEditor;
    viewer->setData(treeData);
    viewer->setReadOnly(true);
    return viewer;
}

//-----------------------------------------------------------------------------
// XML

namespace {
const QString XML_ROOT = QStringLiteral("RootNode");
} // end of anonymous namespace

void GeneralTreeObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    if (!treeData.isEmpty()) {
        xml.writeStartElement(XML_ROOT);
        treeData.saveToXML(xml);
    }
}

GeneralTreeObject* GeneralTreeObject::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    const char* curElement = "GeneralTreeObject";
    if (!xml.readNextStartElement()) {
        return new GeneralTreeObject();
    }

    if (Q_UNLIKELY(xml.name() != XML_ROOT)) {
        XMLError::unexpectedElement(qWarning(), xml, curElement, XML_ROOT);
        return nullptr;
    }

    Tree tree;

    if (Q_UNLIKELY(!tree.loadFromXML(xml, strCache))) {
        return nullptr;
    }

    return new GeneralTreeObject(tree);
}
