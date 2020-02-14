#include "GeneralTreeObject.h"
#include "src/utils/XMLUtilities.h"

#include <QDebug>

GeneralTreeObject::GeneralTreeObject(const ConstructOptions &opt)
    : IntrinsicObject(ObjectType::Data_GeneralTree, opt)
{

}

GeneralTreeObject::GeneralTreeObject(const Tree &tree, const ConstructOptions &opt)
    : IntrinsicObject(ObjectType::Data_GeneralTree, opt), treeData(tree)
{

}

GeneralTreeObject::~GeneralTreeObject()
{

}

//-----------------------------------------------------------------------------
// XML

namespace {
const QString XML_ROOT = QStringLiteral("RootNode");
const QString XML_NODE = QStringLiteral("Node");
const QString XML_TYPE = QStringLiteral("Type");
const QString XML_PARAMETER_LIST = QStringLiteral("ParameterList");
const QString XML_PARAMETER = QStringLiteral("Parameter");
const QString XML_NAME = QStringLiteral("Name");
const QString XML_CHILD_LIST = QStringLiteral("ChildList");
} // end of anonymous namespace

void GeneralTreeObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    if (!treeData.isEmpty()) {
        xml.writeStartElement(XML_ROOT);
        saveToXML(xml, 0);
        xml.writeEndElement();
    }
}

void GeneralTreeObject::saveToXML(QXmlStreamWriter& xml, int nodeIndex)
{
    const Node& curNode = treeData.getNode(nodeIndex);
    Q_ASSERT(curNode.keyList.size() == curNode.valueList.size());
    xml.writeAttribute(XML_TYPE, curNode.typeName);
    {
        xml.writeStartElement(XML_PARAMETER_LIST);
        for (int i = 0, n = curNode.keyList.size(); i < n; ++i) {
            xml.writeStartElement(XML_PARAMETER);
            xml.writeAttribute(XML_NAME, curNode.keyList.at(i));
            xml.writeCharacters(curNode.valueList.at(i));
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    {
        xml.writeStartElement(XML_CHILD_LIST);
        for (int childOffset : curNode.offsetToChildren) {
            xml.writeStartElement(XML_NODE);
            saveToXML(xml, nodeIndex + childOffset);
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

namespace {
bool loadGeneralTreeNodeFromXML(QXmlStreamReader& xml, TreeBuilder& tree, StringCache& strCache, TreeBuilder::Node* parent)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    const char* curElement = "GeneralTreeNode";
    auto ptr = tree.addNode(parent);
    auto& n = *ptr;
    if (Q_UNLIKELY(!XMLUtil::readStringAttribute(xml, curElement, QString(), XML_TYPE, n.typeName, strCache))) {
        return false;
    }
    auto readKVPair = [&](QXmlStreamReader& xml, StringCache& strCache) -> bool {
        QString key;
        QString value;
        if (Q_UNLIKELY(!XMLUtil::readStringAttribute(xml, curElement, QString(), XML_NAME, key, strCache))) {
            return false;
        }
        if (Q_UNLIKELY(!XMLUtil::readElementText(xml, curElement, XML_NAME, value, strCache))) {
            return false;
        }
        n.keyList.push_back(key);
        n.valueList.push_back(value);
        return true;
    };

    if (Q_UNLIKELY(!XMLUtil::readGeneralList(xml, curElement, XML_PARAMETER_LIST, XML_PARAMETER, readKVPair, strCache))) {
        return false;
    }
    auto loadChild = [&](QXmlStreamReader& xml, StringCache& strCache) -> bool {
        return loadGeneralTreeNodeFromXML(xml, tree, strCache, ptr);
    };

    if (Q_UNLIKELY(!XMLUtil::readGeneralList(
                       xml, curElement, XML_CHILD_LIST, XML_NODE,
                       loadChild,
                       strCache))) {
        return false;
    }

    xml.skipCurrentElement();
    return true;
}
}

GeneralTreeObject* GeneralTreeObject::loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    const char* curElement = "GeneralTreeObject";
    if (!xml.readNextStartElement()) {
        return new GeneralTreeObject(opt);
    }

    if (Q_UNLIKELY(xml.name() != XML_ROOT)) {
        XMLError::unexpectedElement(qWarning(), xml, curElement, XML_ROOT);
        return nullptr;
    }

    TreeBuilder tree;

    if (Q_UNLIKELY(!loadGeneralTreeNodeFromXML(xml, tree, strCache, nullptr))) {
        return nullptr;
    }

    Tree* treePtr = nullptr;
    try {
        treePtr = new Tree(tree);
    } catch (...) {
        // do nothing
    }
    if (treePtr) {
        GeneralTreeObject* result = new GeneralTreeObject(*treePtr, opt);
        delete treePtr;
        return result;
    }
    return nullptr;
}
