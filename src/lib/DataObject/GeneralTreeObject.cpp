#include "GeneralTreeObject.h"

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
const QString XML_NODE = QStringLiteral("Node");
const QString XML_TYPE = QStringLiteral("Type");
const QString XML_PARAMETER_LIST = QStringLiteral("ParameterList");
const QString XML_PARAMETER = QStringLiteral("Parameter");
const QString XML_NAME = QStringLiteral("Name");
const QString XML_CHILD_LIST = QStringLiteral("ChildList");
} // end of anonymous namespace

void GeneralTreeObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    xml.writeStartElement(getTypeClassName());
    if (!treeData.isEmpty())
        saveToXML(xml, 0);
    xml.writeEndElement();
}

void GeneralTreeObject::saveToXML(QXmlStreamWriter& xml, int nodeIndex)
{
    const Node& curNode = treeData.getNode(nodeIndex);
    Q_ASSERT(curNode.keyList.size() == curNode.valueList.size());
    xml.writeStartElement(XML_NODE);
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
            saveToXML(xml, nodeIndex + childOffset);
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

namespace {
bool loadGeneralTreeNodeFromXML(QXmlStreamReader& xml, TreeBuilder& tree, QHash<QStringRef,QString>& strCache, TreeBuilder::Node* parent)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    auto getString = [] (QHash<QStringRef,QString>& cache, QStringRef str) -> QString {
        auto iter = cache.find(str);
        if (iter == cache.end()) {
            QString result = str.toString();
            cache.insert(str, result);
            return result;
        }
        return iter.value();
    };
    if (Q_UNLIKELY(xml.name() != XML_NODE)) {
        qWarning() << "Unexpected element " << xml.name() << "(expecting " << XML_NODE <<")";
        return false;
    }

    auto ptr = tree.addNode(parent);
    auto& n = *ptr;
    {
        auto attr = xml.attributes();
        if (Q_UNLIKELY(!attr.hasAttribute(XML_TYPE))) {
            qWarning() << "Missing " << XML_TYPE << "attribute on " << xml.name() << "element";
            return false;
        }
        n.typeName = getString(strCache, attr.value(XML_TYPE));
    }
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "Missing " << XML_PARAMETER_LIST << " element in " << XML_NODE << "element";
        return false;
    }

    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    if (Q_UNLIKELY(xml.name() != XML_PARAMETER_LIST)) {
        qWarning() << "Unexpected element " << xml.name() << "(expecting " << XML_PARAMETER_LIST <<")";
        return false;
    }
    while (xml.readNextStartElement()) {
        if (Q_UNLIKELY(xml.name() != XML_PARAMETER)) {
            qWarning() << "Unexpected element " << xml.name() << "(expecting " << XML_PARAMETER <<")";
            return false;
        }
        auto attr = xml.attributes();
        QString key;
        QString value;
        if (Q_UNLIKELY(!attr.hasAttribute(XML_NAME))) {
            qWarning() << "Missing " << XML_NAME << "attribute on " << xml.name() << "element";
            return false;
        }
        key = getString(strCache, attr.value(XML_NAME));
        if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
            qWarning() << "Unexpected token " << xml.tokenString() << "(Expecting Characters)";
            return false;
        }
        value = getString(strCache, xml.text());
        if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
            qWarning() << "Unexpected token " << xml.tokenString() << "(Expecting EndElement)";
            return false;
        }
        if (Q_UNLIKELY(xml.hasError())) {
            qWarning() << "Error when reading parameter value:" << xml.errorString();
            return false;
        }
        n.keyList.push_back(key);
        n.valueList.push_back(value);
    }
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == XML_PARAMETER_LIST);

    if (Q_UNLIKELY(xml.hasError())) {
        qWarning() << "Error on reading parameter list: " << xml.errorString();
        return false;
    }

    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "Missing " << XML_CHILD_LIST << " element in " << XML_NODE << "element";
        return false;
    }

    while (xml.readNextStartElement()) {
        if (Q_UNLIKELY(!loadGeneralTreeNodeFromXML(xml, tree, strCache, ptr))) {
            return false;
        }
    }

    Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == XML_CHILD_LIST);
    auto end = xml.readNext();
    while (end == QXmlStreamReader::Characters || end == QXmlStreamReader::Comment) {
        end = xml.readNext();
    }
    Q_ASSERT(end == QXmlStreamReader::EndElement && xml.name() == XML_NODE);
    return true;
}
}

GeneralTreeObject* GeneralTreeObject::loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    QString top = getTypeClassName(ObjectType::Data_GeneralTree);
    if (Q_UNLIKELY(xml.name() != top)) {
        qWarning() << "Unexpected element " << xml.name() << "(expecting " << top <<")";
        return nullptr;
    }
    TreeBuilder tree;

    if (xml.readNextStartElement()) {
        QHash<QStringRef,QString> strCache;
        if (Q_UNLIKELY(!loadGeneralTreeNodeFromXML(xml, tree, strCache, nullptr))) {
            return nullptr;
        }
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
