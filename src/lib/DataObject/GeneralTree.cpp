#include "GeneralTree.h"

#include <QDebug>

GeneralTree::GeneralTree(const ConstructOptions &opt)
    : IntrinsicObject(ObjectType::GeneralTree, opt)
{

}

GeneralTree::GeneralTree(const QList<GeneralTree::Node>& _nodes, const ConstructOptions &opt)
    : IntrinsicObject(ObjectType::GeneralTree, opt), nodes(_nodes)
{

}

GeneralTree::~GeneralTree()
{
    if (ui) {
        ui->hide();
        ui->deleteLater();
    }
}

TextEditor* GeneralTree::getEditor()
{
    if (!ui) {
        ui = new TextEditor;
        Q_ASSERT(ui);
        QString text;
        QXmlStreamWriter xml(&text);
        IntrinsicObject::saveToXML(xml);
        ui->setPlainText(text);
        ui->setReadOnly(true);
    }
    return ui;
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

void GeneralTree::saveToXMLImpl(QXmlStreamWriter &xml)
{
    xml.writeStartElement(getTypeClassName());
    if (!nodes.isEmpty())
        saveToXML(xml, 0);
    xml.writeEndElement();
}

void GeneralTree::saveToXML(QXmlStreamWriter& xml, int nodeIndex)
{
    const Node& curNode = nodes.at(nodeIndex);
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
        for (int child : curNode.children) {
            saveToXML(xml, child);
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

namespace {
bool loadGeneralTreeNodeFromXML(QXmlStreamReader& xml, QList<GeneralTree::Node>& nodes, QHash<QStringRef,QString>& strCache, int parent)
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

    GeneralTree::Node n;
    n.parentIndex = parent;
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
    int nodeIndex = nodes.size();
    nodes.push_back(n);

    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "Missing " << XML_CHILD_LIST << " element in " << XML_NODE << "element";
        return false;
    }

    int childCount = 0;
    while (xml.readNextStartElement()) {
        childCount += 1;
        if (Q_UNLIKELY(!loadGeneralTreeNodeFromXML(xml, nodes, strCache, nodeIndex))) {
            return false;
        }
    }

    Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == XML_CHILD_LIST);
    auto end = xml.readNext();
    while (end == QXmlStreamReader::Characters || end == QXmlStreamReader::Comment) {
        end = xml.readNext();
    }
    Q_ASSERT(end == QXmlStreamReader::EndElement && xml.name() == XML_NODE);
    nodes[nodeIndex].children.reserve(childCount);
    return true;
}
}

GeneralTree* GeneralTree::loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt)
{
    // TODO
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    QString top = getTypeClassName(ObjectType::GeneralTree);
    if (Q_UNLIKELY(xml.name() != top)) {
        qWarning() << "Unexpected element " << xml.name() << "(expecting " << top <<")";
        return nullptr;
    }
    QList<Node> nodes;

    // first pass: get all nodes
    if (xml.readNextStartElement()) {
        QHash<QStringRef,QString> strCache;
        if (Q_UNLIKELY(!loadGeneralTreeNodeFromXML(xml, nodes, strCache, -1))) {
            return nullptr;
        }
    }
    // second pass: populate child lists
    for (int i = 0, n = nodes.size(); i < n; ++i) {
        int parent = nodes.at(i).parentIndex;
        Q_ASSERT(parent < n);
        if (parent >= 0) {
            nodes[parent].children.push_back(i);
        }
    }

    return new GeneralTree(nodes, opt);
}
