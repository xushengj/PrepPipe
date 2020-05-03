#include "src/lib/Tree/SimpleTreeTransform.h"
#include "src/utils/NameSorting.h"
#include <QDebug>

namespace {
void performTransformImpl(
        const SimpleTreeTransform::Data& transform,
        const Tree& tree,
        TreeBuilder& builder,
        const QList<const Tree*>& sideTreeList,
        QVector<int>& skipSrcVec,
        QVector<SimpleTreeTransform::NodeProvenance>& provenanceVec,
        QVector<SimpleTreeTransform::TransformError>& errors,
        int startNode,
        TreeBuilder::Node* parent)
{
    Q_ASSERT(sideTreeList.size() == transform.sideTreeNameList.size());
    const Tree::Node& node = tree.getNode(startNode);
    int patternIndex = -1;
    const SimpleTreeTransform::NodeTransformRule* patternPtr = nullptr;
    Tree::EvaluationContext ctx(tree, sideTreeList, startNode);
    bool isRemovedBecauseSkipped = (skipSrcVec.at(startNode) >= 0);
    bool isNodeTypeRecognized = false;
    if (!isRemovedBecauseSkipped) {
        auto iter = transform.nodeTypeToRuleList.find(node.typeName);
        if (iter != transform.nodeTypeToRuleList.end()) {
            isNodeTypeRecognized = true;
            for (int i = 0, n = iter.value().size(); i < n; ++i) {
                const auto& pattern = iter.value().at(i);
                bool isAnyPredicateFailed = false;
                for (const auto& pred : pattern.predicates) {
                    bool isGood = false;
                    if (!(Tree::evaluatePredicate(pred, isGood, ctx) && isGood)) {
                        isAnyPredicateFailed = true;
                        break;
                    }
                }
                if (!isAnyPredicateFailed) {
                    // we get a pattern
                    patternIndex = i;
                    patternPtr = &pattern;
                    break;
                }
            }
        }
    }

    if (patternPtr) {
        // mark all nodes destined at skipNodes for skipping
        // this have to be before applying decision so that skipping child node is visible in recursion
        for (const auto& nodePath : patternPtr->skipNodes) {
            bool isGood = false;
            int destNodeIndex = tree.nodeTraverse(startNode, node, nodePath, isGood);
            if (isGood) {
                if (destNodeIndex > startNode) {
                    if (skipSrcVec.at(destNodeIndex) < 0) {
                        skipSrcVec[destNodeIndex] = startNode;
                    }
                }
            }
        }
    }

    decltype(patternPtr->ty) decision = SimpleTreeTransform::NodeTransformRule::TransformType::PassThrough;

    if (isRemovedBecauseSkipped) {
        decision = SimpleTreeTransform::NodeTransformRule::TransformType::Remove;
    } else if (!isNodeTypeRecognized) {
        // unrecognized node
        decltype (transform.unrecognizedNodeActionOverride) act = SimpleTreeTransform::PredefinedAction::PassThrough;
        if (transform.isUnrecognizedNodeUseDefaultAction) {
            act = transform.defaultAction;
        } else {
            act = transform.unrecognizedNodeActionOverride;
        }
        switch (act) {
        case SimpleTreeTransform::PredefinedAction::PassThrough:
            decision = SimpleTreeTransform::NodeTransformRule::TransformType::PassThrough;
            break;
        case SimpleTreeTransform::PredefinedAction::Remove:
            decision = SimpleTreeTransform::NodeTransformRule::TransformType::Remove;
            break;
        case SimpleTreeTransform::PredefinedAction::Error: {
            SimpleTreeTransform::TransformError err;
            err.srcNodeIndex = startNode;
            err.patternIndex = patternIndex;
            err.cause = SimpleTreeTransform::TransformError::Cause::RequestedError_UnrecognizedNodeType;
            err.unrecognizedNodeType = node.typeName;
            errors.push_back(err);
            decision = SimpleTreeTransform::NodeTransformRule::TransformType::Remove;
        }break;
        }
    } else if (patternIndex == -1) {
        // default action
        switch (transform.defaultAction) {
        case SimpleTreeTransform::PredefinedAction::PassThrough:
            decision = SimpleTreeTransform::NodeTransformRule::TransformType::PassThrough;
            break;
        case SimpleTreeTransform::PredefinedAction::Remove:
            decision = SimpleTreeTransform::NodeTransformRule::TransformType::Remove;
            break;
        case SimpleTreeTransform::PredefinedAction::Error: {
            SimpleTreeTransform::TransformError err;
            err.srcNodeIndex = startNode;
            err.patternIndex = patternIndex;
            err.cause = SimpleTreeTransform::TransformError::Cause::RequestedError_DefaultActionForNoRuleMatch;
            errors.push_back(err);
            decision = SimpleTreeTransform::NodeTransformRule::TransformType::Remove;
        }break;
        }
    } else {
        Q_ASSERT(patternPtr);
        decision = patternPtr->ty;
    }

    switch (decision) {
    case SimpleTreeTransform::NodeTransformRule::TransformType::PassThrough: {
        TreeBuilder::Node* newNode = builder.addNode(parent);
        Q_ASSERT(newNode->getSequenceNumber() == provenanceVec.size());
        newNode->setDataFromNode(node);
        SimpleTreeTransform::NodeProvenance src;
        src.srcNodeIndex = startNode;
        src.patternIndex = -1;
        provenanceVec.push_back(src);
        for (int childOffset : node.offsetToChildren) {
            performTransformImpl(transform, tree, builder, sideTreeList, skipSrcVec, provenanceVec, errors, startNode + childOffset, newNode);
        }
    }break;
    case SimpleTreeTransform::NodeTransformRule::TransformType::Remove: {
        // do nothing
    }break;
    case SimpleTreeTransform::NodeTransformRule::TransformType::Replace: {
        QList<TreeBuilder::Node*> newNodes;
        SimpleTreeTransform::NodeProvenance src;
        src.srcNodeIndex = startNode;
        src.patternIndex = patternIndex;
        bool isGood = true;
        for (const auto& nodeTemplate : patternPtr->nodeTemplates) {
            QString ty = Tree::evaluateGeneralValueExpression(nodeTemplate.ty, isGood, ctx);
            if (!isGood) {
                SimpleTreeTransform::TransformError err;
                err.srcNodeIndex = startNode;
                err.patternIndex = patternIndex;
                err.cause = SimpleTreeTransform::TransformError::Cause::EvaluationFail_NodeTemplate_NodeType;
                errors.push_back(err);
                break;
            }
            QStringList keyList;
            QStringList valueList;
            for (int i = 0, n = nodeTemplate.kvList.size(); i < n; ++i) {
                const auto& kvPair = nodeTemplate.kvList.at(i);
                bool isKeyGood = false;
                bool isValueGood = false;
                QString key = Tree::evaluateGeneralValueExpression(kvPair.key, isKeyGood, ctx);
                QString value = Tree::evaluateGeneralValueExpression(kvPair.value, isValueGood, ctx);
                if (isKeyGood && isValueGood) {
                    keyList.push_back(key);
                    valueList.push_back(value);
                } else {
                    isGood = false;
                    SimpleTreeTransform::TransformError err;
                    err.srcNodeIndex = startNode;
                    err.patternIndex = patternIndex;
                    err.cause = SimpleTreeTransform::TransformError::Cause::EvaluationFail_NodeTemplate_KeyValue;
                    err.evalFailData.key = key;
                    err.evalFailData.value = value;
                    err.evalFailData.keyValueIndex = i;
                    err.evalFailData.isKeyGood = isKeyGood;
                    err.evalFailData.isValueGood = isValueGood;
                    errors.push_back(err);
                    break;
                }
            }
            if (isGood) {
                TreeBuilder::Node* parentForNewNode = (nodeTemplate.parentIndex == -1)? parent : newNodes.at(nodeTemplate.parentIndex);
                TreeBuilder::Node* newNode = builder.addNode(parentForNewNode);
                Q_ASSERT(newNode->getSequenceNumber() == provenanceVec.size());
                provenanceVec.push_back(src);
                newNodes.push_back(newNode);
                if (ty.isEmpty() && keyList.isEmpty() /* && valueList.isEmpty() */) {
                    newNode->setDataFromNode(node);
                } else {
                    newNode->typeName = ty;
                    newNode->keyList = keyList;
                    newNode->valueList = valueList;
                }
            } else {
                // we already have a failure; cannot promise the tree to be reconstructed fine
                break;
            }
        }
        // we do not recurse into children for replace type
    }break;
    case SimpleTreeTransform::NodeTransformRule::TransformType::Modify: {
        TreeBuilder::Node* newNode = builder.addNode(parent);
        Q_ASSERT(newNode->getSequenceNumber() == provenanceVec.size());
        newNode->setDataFromNode(node);
        SimpleTreeTransform::NodeProvenance src;
        src.srcNodeIndex = startNode;
        src.patternIndex = -1;
        provenanceVec.push_back(src);

        // apply patches to this node
        for (int i = 0, n = patternPtr->modifications.size(); i < n; ++i) {
            const auto& patch = patternPtr->modifications.at(i);
            bool isKeyGood = false;
            bool isValueGood = false;
            QString key = Tree::evaluateGeneralValueExpression(patch.key, isKeyGood, ctx);
            QString value = Tree::evaluateGeneralValueExpression(patch.value, isValueGood, ctx);
            if (isKeyGood && isValueGood) {
                if (key.isEmpty()) {
                    // we are modifying the type
                    newNode->typeName = value;
                } else {
                    int keyIndex = newNode->keyList.indexOf(key);
                    if (keyIndex == -1) {
                        newNode->keyList.push_back(key);
                        newNode->valueList.push_back(value);
                    } else {
                        // add a check that's basically only to ensure that keyIndex is in bound.
                        if (newNode->valueList.at(keyIndex) != value) {
                            newNode->valueList[keyIndex] = value;
                        }
                    }
                }
            } else {
                // either key or value (or both) evaluation failed
                // skip current patch
                SimpleTreeTransform::TransformError err;
                err.srcNodeIndex = startNode;
                err.patternIndex = patternIndex;
                err.cause = SimpleTreeTransform::TransformError::Cause::EvaluationFail_Modification;
                err.evalFailData.key = key;
                err.evalFailData.value = value;
                err.evalFailData.keyValueIndex = i;
                err.evalFailData.isKeyGood = isKeyGood;
                err.evalFailData.isValueGood = isValueGood;
                errors.push_back(err);
                break;
            }
        }

        for (int childOffset : node.offsetToChildren) {
            performTransformImpl(transform, tree, builder, sideTreeList, skipSrcVec, provenanceVec, errors, startNode + childOffset, newNode);
        }
    }break;
    }
}

} // end of anonymous namespace

bool SimpleTreeTransform::performTransform(const Tree& tree, Tree& dest, const QList<const Tree*>& sideTreeList) const
{
    int numNodes = tree.getNumNodes();
    if (Q_UNLIKELY(numNodes == 0)) {
        return true;
    }
    // for each node in source tree, which source node and pattern index cause it to be skipped
    QVector<int> skipSrcVec(numNodes, -1);
    QVector<NodeProvenance> srcVec;
    QVector<TransformError> errors;
    TreeBuilder builder;
    srcVec.reserve(numNodes);
    performTransformImpl(data, tree, builder, sideTreeList, skipSrcVec, srcVec, errors, 0, nullptr);
    QVector<int> seqTable;
    Tree newTree(builder, seqTable);
    dest.swap(newTree);
    return (errors.isEmpty());
}

namespace {
const QString XML_DEFAULT_ACTION = QStringLiteral("DefaultAction");
const QString XML_ACTION_PASSTHROUGH = QStringLiteral("PassThrough");
const QString XML_ACTION_REMOVE = QStringLiteral("Remove");
const QString XML_ACTION_ERROR = QStringLiteral("Error");
// not in PredefinedAction but in ModificationFlag
const QString XML_ACTION_REPLACE = QStringLiteral("Replace");
const QString XML_ACTION_MODIFY = QStringLiteral("Modify");

const QString XML_UNRECOGNIZED_NODE_ACTION = QStringLiteral("UnrecognizedNodeAction");
const QString XML_UNRECOGNIZED_NODE_DEFAULTED = QStringLiteral("Default");

const QString XML_SIDE_TREE_LIST = QStringLiteral("SideTrees");
const QString XML_SIDE_TREE = QStringLiteral("SideTree");
const QString XML_NODE_TRANSFORM_LIST = QStringLiteral("NodeTransforms");
const QString XML_NODE_TRANSFORM = QStringLiteral("NodeTransform");
const QString XML_NODE_TRANSFORM_NODE_TYPE = QStringLiteral("NodeType");
const QString XML_NODE_TRANSFORM_RULE = QStringLiteral("TransformRule");
} // end of anonymous namespace

void SimpleTreeTransform::Data::saveToXML(QXmlStreamWriter& xml) const
{
    auto getPredefinedActionStr = [](PredefinedAction act) -> const QString& {
        switch (act) {
        case PredefinedAction::PassThrough: return XML_ACTION_PASSTHROUGH;
        case PredefinedAction::Remove:      return XML_ACTION_REMOVE;
        case PredefinedAction::Error:       return XML_ACTION_ERROR;
        }
        Q_UNREACHABLE();
    };
    xml.writeTextElement(XML_DEFAULT_ACTION, getPredefinedActionStr(defaultAction));

    if (isUnrecognizedNodeUseDefaultAction) {
        xml.writeTextElement(XML_UNRECOGNIZED_NODE_ACTION, XML_UNRECOGNIZED_NODE_DEFAULTED);
    } else {
        xml.writeTextElement(XML_UNRECOGNIZED_NODE_ACTION, getPredefinedActionStr(unrecognizedNodeActionOverride));
    }

    xml.writeStartElement(XML_SIDE_TREE_LIST);
    for (const auto& tree : sideTreeNameList) {
        xml.writeTextElement(XML_SIDE_TREE, tree);
    }
    xml.writeEndElement();

    xml.writeStartElement(XML_NODE_TRANSFORM_LIST);
    if (!nodeTypeToRuleList.isEmpty()) {
        QStringList sortedNodeTypeList = nodeTypeToRuleList.keys();
        NameSorting::sortNameList(sortedNodeTypeList);
        for (const auto& tyName : sortedNodeTypeList) {
            auto iter = nodeTypeToRuleList.find(tyName);
            Q_ASSERT(iter != nodeTypeToRuleList.end());
            xml.writeStartElement(XML_NODE_TRANSFORM);
            xml.writeAttribute(XML_NODE_TRANSFORM_NODE_TYPE, tyName);
            for (const auto& rule : iter.value()) {
                xml.writeStartElement(XML_NODE_TRANSFORM_RULE);
                rule.saveToXML(xml);
            }
            xml.writeEndElement();
        }
    }
    xml.writeEndElement();
}

namespace {

bool readNodeTransform(QHash<QString, QVector<SimpleTreeTransform::NodeTransformRule>>& dest, QXmlStreamReader& xml, StringCache& strCache) {
    QString tyName;
    const char* curElement = "NodeTransform";
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    Q_ASSERT(xml.name() == XML_NODE_TRANSFORM);
    if (Q_UNLIKELY(!XMLUtil::readAttribute(
        xml, curElement, XML_NODE_TRANSFORM, XML_NODE_TRANSFORM_NODE_TYPE,
        [&] (QStringRef str) ->bool {
            tyName = strCache(str);
            return true;
        }, {}, nullptr)))
    {
        return false;
    }
    QVector<SimpleTreeTransform::NodeTransformRule> ruleList;
    while (xml.readNextStartElement()) {
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
        if (Q_UNLIKELY(xml.name() != XML_NODE_TRANSFORM_RULE)) {
            return XMLError::unexpectedElement(qWarning(), xml, curElement, XML_NODE_TRANSFORM_RULE);
        }
        SimpleTreeTransform::NodeTransformRule rule;
        if (Q_UNLIKELY(!rule.loadFromXML(xml, strCache))) {
            return XMLError::failOnChild(qWarning(), curElement, XML_NODE_TRANSFORM_RULE);
        }
        ruleList.push_back(rule);
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, curElement, XML_NODE_TRANSFORM);
    }
    dest.insert(tyName, ruleList);
    return true;
}
} // end of anonymous namespace

bool SimpleTreeTransform::Data::loadFromXML(QXmlStreamReader& xml, StringCache &strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    auto setAction = [](QStringRef act, SimpleTreeTransform::PredefinedAction& result) -> bool {
        if (act == XML_ACTION_PASSTHROUGH) {
            result = PredefinedAction::PassThrough;
            return true;
        }
        if (act == XML_ACTION_REMOVE) {
            result = PredefinedAction::Remove;
            return true;
        }
        if (act == XML_ACTION_ERROR) {
            result = PredefinedAction::Error;
            return true;
        }
        return false;
    };
    const char* curElement = "SimpleTreeTransform";

    // default action
    if (Q_UNLIKELY(!XMLUtil::readEnum(
        xml, curElement, XML_DEFAULT_ACTION, std::bind(setAction, std::placeholders::_1, defaultAction),
    {
        XML_ACTION_PASSTHROUGH,
        XML_ACTION_REMOVE,
        XML_ACTION_ERROR
    }))) {
        return false;
    }

    // unrecognized node action
    if (Q_UNLIKELY(!XMLUtil::readEnum(
        xml, curElement, XML_UNRECOGNIZED_NODE_ACTION,
        [=](QStringRef str) -> bool
    {
        if (str == XML_UNRECOGNIZED_NODE_DEFAULTED) {
            isUnrecognizedNodeUseDefaultAction = true;
            return true;
        }else {
            isUnrecognizedNodeUseDefaultAction = false;
            return setAction(str, unrecognizedNodeActionOverride);
        }
    },
    {
        XML_UNRECOGNIZED_NODE_DEFAULTED,
        XML_ACTION_PASSTHROUGH,
        XML_ACTION_REMOVE,
        XML_ACTION_ERROR
    }))){
        return false;
    }


    // side tree list
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_SIDE_TREE_LIST, XML_SIDE_TREE, sideTreeNameList, strCache))) {
        return false;
    }

    // transform rules
    nodeTypeToRuleList.clear();
    if (Q_UNLIKELY(!XMLUtil::readGeneralList(xml, curElement, XML_NODE_TRANSFORM_LIST, XML_NODE_TRANSFORM, std::bind(readNodeTransform, std::ref(nodeTypeToRuleList), std::placeholders::_1, std::placeholders::_2), strCache))) {
        return false;
    }

    xml.skipCurrentElement();
    return true;
}

namespace  {
const QString XML_TRANSFORM_TYPE = QStringLiteral("TransformType");
const QString XML_PREDICATE_LIST = QStringLiteral("Predicates");
const QString XML_PREDICATE = QStringLiteral("Predicate");
const QString XML_NODE_TEMPLATE_LIST = QStringLiteral("NodeTemplates");
const QString XML_NODE_TEMPLATE = QStringLiteral("NodeTemplate");
const QString XML_PARENT_INDEX = QStringLiteral("ParentIndex");
const QString XML_NODE_TYPE = QStringLiteral("NodeType");
const QString XML_KEYVALUE_LIST = QStringLiteral("KeyValueList");
const QString XML_KEYVALUE = QStringLiteral("KeyValue");
const QString XML_PATCH_LIST = QStringLiteral("Modifications");
const QString XML_PATCH = QStringLiteral("Change");
} // end of anonymous namespace

void SimpleTreeTransform::NodeTransformRule::saveToXML(QXmlStreamWriter& xml) const
{
    switch (ty) {
    case TransformType::PassThrough: {
        xml.writeAttribute(XML_TRANSFORM_TYPE, XML_ACTION_PASSTHROUGH);
    }break;
    case TransformType::Remove: {
        xml.writeAttribute(XML_TRANSFORM_TYPE, XML_ACTION_REMOVE);
    }break;
    case TransformType::Replace: {
        xml.writeAttribute(XML_TRANSFORM_TYPE, XML_ACTION_REPLACE);
    }break;
    case TransformType::Modify: {
        xml.writeAttribute(XML_TRANSFORM_TYPE, XML_ACTION_MODIFY);
    }break;
    }


    xml.writeStartElement(XML_PREDICATE_LIST);
    for (const auto& pred : predicates) {
        xml.writeStartElement(XML_PREDICATE);
        pred.saveToXML(xml);
    }
    xml.writeEndElement();

    if (ty == TransformType::Replace) {
        xml.writeStartElement(XML_NODE_TEMPLATE_LIST);
        for (int i = 0, n = nodeTemplates.size(); i < n; ++i) {
            const auto& node = nodeTemplates.at(i);
            xml.writeStartElement(XML_NODE_TEMPLATE);
            node.saveToXML(xml);
        }
        xml.writeEndElement();
    } else if (ty == TransformType::Modify) {
        xml.writeStartElement(XML_PATCH_LIST);
        for (int i = 0, n = modifications.size(); i < n; ++i) {
            xml.writeStartElement(XML_PATCH);
            modifications.at(i).saveToXML(xml);
        }
        xml.writeEndElement();
    }

    // done
    xml.writeEndElement();
}

bool SimpleTreeTransform::NodeTransformRule::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    auto setTransformType = [](QStringRef act, SimpleTreeTransform::NodeTransformRule::TransformType& result) -> bool {
        if (act == XML_ACTION_PASSTHROUGH) {
            result = TransformType::PassThrough;
            return true;
        }
        if (act == XML_ACTION_REMOVE) {
            result = TransformType::Remove;
            return true;
        }
        if (act == XML_ACTION_REPLACE) {
            result = TransformType::Replace;
            return true;
        }
        if (act == XML_ACTION_MODIFY) {
            result = TransformType::Modify;
            return true;
        }
        return false;
    };
    const char* curElement = "SimpleTreeTransform::NodeTransformRule";
    if (Q_UNLIKELY(!XMLUtil::readAttribute(
        xml, curElement, QString(), XML_TRANSFORM_TYPE, std::bind(setTransformType, std::placeholders::_1, std::ref(ty)),
    {
        XML_ACTION_PASSTHROUGH,
        XML_ACTION_REMOVE,
        XML_ACTION_REPLACE,
        XML_ACTION_MODIFY
    }, nullptr))) {
        return false;
    }

    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_PREDICATE_LIST, XML_PREDICATE, predicates, strCache))) {
        return false;
    }

    nodeTemplates.clear();
    modifications.clear();
    if (ty == TransformType::Replace) {
        auto addNodeTemplate = [this](QXmlStreamReader& xml, StringCache& strCache) -> bool {
            nodeTemplates.push_back(SubTreeNodeTemplate());
            auto& node = nodeTemplates.back();
            return node.loadFromXML(xml, strCache) && (node.parentIndex < nodeTemplates.size());
        };
        if (Q_UNLIKELY(!XMLUtil::readGeneralList(xml, curElement, XML_NODE_TEMPLATE_LIST, XML_NODE_TEMPLATE, addNodeTemplate, strCache))) {
            return false;
        }
    } else if (ty == TransformType::Modify) {
        if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_PATCH_LIST, XML_PATCH, modifications, strCache))) {
            return false;
        }
    }

    xml.skipCurrentElement();
    return true;
}

void SimpleTreeTransform::SubTreeNodeTemplate::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeAttribute(XML_PARENT_INDEX, QString::number(parentIndex));
    {
        xml.writeStartElement(XML_NODE_TYPE);
        ty.saveToXML(xml);
    }
    {
        xml.writeStartElement(XML_KEYVALUE_LIST);
        for (const auto& p : kvList) {
            xml.writeStartElement(XML_KEYVALUE);
            p.saveToXML(xml);
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

bool SimpleTreeTransform::SubTreeNodeTemplate::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    auto readParentIndex = [this](QStringRef str) -> bool {
        bool isGood = false;
        parentIndex = str.toInt(&isGood);
        return (isGood && parentIndex >= -1);
    };
    const char* curElement = "SimpleTreeTransform::SubTreeNodeTemplate";
    if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_PARENT_INDEX, readParentIndex, {}, "Invalid value (should be an integer >= -1)"))) {
        return false;
    }

    if (Q_UNLIKELY(!XMLUtil::readLoadable(xml, curElement, XML_NODE_TYPE, ty, strCache))) {
        return false;
    }

    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_KEYVALUE_LIST, XML_KEYVALUE, kvList, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

namespace  {
const QString XML_KEY = QStringLiteral("Key");
const QString XML_VALUE = QStringLiteral("Value");
} // end of anonymous namespace
void SimpleTreeTransform::KeyValueExpressionPair::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeStartElement(XML_KEY);
    key.saveToXML(xml);
    xml.writeStartElement(XML_VALUE);
    value.saveToXML(xml);
}
bool SimpleTreeTransform::KeyValueExpressionPair::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleTreeTransform::KeyValueExpressionPair";
    if (Q_UNLIKELY(!XMLUtil::readLoadable(xml, curElement, XML_KEY, key, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadable(xml, curElement, XML_VALUE, value, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}
