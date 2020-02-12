#include "src/lib/Tree/Tree.h"

#include <QDebug>

#include <stdexcept>
#include <vector>
#include <algorithm>

QString Tree::evaluateLocalValueExpression(const Tree::Node& startNode, const LocalValueExpression& expr, bool& isGood)
{
    isGood = true;
    switch (expr.ty) {
    case LocalValueExpression::ValueType::KeyValue: {
        int index = startNode.keyList.indexOf(expr.str);
        if (index == -1) {
            // no such key
            isGood = false;
            return QString();
        }
        return startNode.valueList.at(index);
    }
    case LocalValueExpression::ValueType::Literal: {
        return expr.str;
    }
    case LocalValueExpression::ValueType::NodeType: {
        return startNode.typeName;
    }
    } // end of switch (should be unreachable afterwards)
    qFatal("unhandled type");
    return QString();
}

int Tree::nodeTraverse(int currentNodeIndex, const Node& localValueEvaluationNode, const NodeTraverseStep& step, bool &isGood) const
{
    const auto& currentNode = nodes.at(currentNodeIndex);

    if (step.destination == NodeTraverseStep::StepDestination::Parent) {
        // simplest case
        // going up at root node is considered a failure
        isGood = (currentNode.offsetFromParent > 0);
        return currentNodeIndex - currentNode.offsetFromParent;
    }

    std::vector<int> candidates;
    switch (step.destination) {
    case NodeTraverseStep::StepDestination::Parent: Q_UNREACHABLE();
    case NodeTraverseStep::StepDestination::Peer: {
        if (currentNode.offsetFromParent == 0) {
            // root node has no other peer
            candidates.push_back(currentNodeIndex);
        } else {
            // get all other peers
            int parentIndex = currentNodeIndex - currentNode.offsetFromParent;
            const auto& parent = nodes.at(parentIndex);
            candidates.reserve(static_cast<std::size_t>(parent.offsetToChildren.size()));
            for (int childOffset : parent.offsetToChildren) {
                candidates.push_back(parentIndex + childOffset);
            }
        }
    }break;
    case NodeTraverseStep::StepDestination::Child: {
        candidates.reserve(static_cast<std::size_t>(currentNode.offsetToChildren.size()));
        for (int childOffset : currentNode.offsetToChildren) {
            candidates.push_back(currentNodeIndex + childOffset);
        }
    }
    }

    if (candidates.empty()) {
        isGood = false;
        return currentNodeIndex;
    }

    // preprocess key value type filter so that we don't have to consult start node all the time
    QHash<QString, QString> keyValueFilter;
    for (auto iter = step.keyValueFilter.begin(), iterEnd = step.keyValueFilter.end(); iter != iterEnd; ++iter) {
        keyValueFilter.insert(iter.key(), evaluateLocalValueExpression(localValueEvaluationNode, iter.value(), isGood));
        // here we don't care whether the expression evaluation is good; we just accept empty string upon failure
    }

    // apply filters to candidates
    decltype(candidates) tmpList;
    tmpList.swap(candidates);
    for (int candidate : tmpList) {
        const auto& node = nodes.at(candidate);

        // check type filter
        if (!step.childTypeFilter.isEmpty()) {
            if (node.typeName != step.childTypeFilter)
                continue;
        }

        // check key value filter
        bool isKeyValueCheckFailed = false;
        for (auto iter = keyValueFilter.begin(), iterEnd = keyValueFilter.end(); iter != iterEnd; ++iter) {
            int index = node.keyList.indexOf(iter.key());
            // note: this means that when applying the filter,
            // there is no difference between "there is a key-value pair with empty value"
            // and "there is no such a key-value pair with given key"
            if ((index == -1 && !iter.value().isEmpty()) || (node.valueList.at(index) != iter.value())) {
                isKeyValueCheckFailed = true;
                break;
            }
        }
        if (isKeyValueCheckFailed)
            continue;

        // okay, all checks has passed
        candidates.push_back(candidate);
    }
    // tmpList is no longer used after this point

    if (candidates.empty()) {
        isGood = false;
        return currentNodeIndex;
    }

    // determine which result to use
    if (step.offsetDeterminer != 0) {
        if (step.destination != NodeTraverseStep::StepDestination::Peer) {
            // determine by offset is only supported in peer mode
            isGood = false;
            return currentNodeIndex;
        }

        auto iter = std::lower_bound(candidates.begin(), candidates.end(), currentNodeIndex);

        // since lower_bound may return the exact position of startNode if it is also a candidate,
        // we want to increment iter by 1 if that's the case
        if (step.offsetDeterminer > 0) {
            // if the start node is already after everything, looking for "next" node is a guaranteed fail
            if (iter == candidates.end()) {
                isGood = false;
                return currentNodeIndex;
            }
            // if step.offsetDeterminer is 1, we want "next" node, no matter whether start node is one of the candidate
            if (*iter == currentNodeIndex) {
                ++iter;
            }
        }
        auto startNodeRoughIndex = std::distance(candidates.begin(), iter);
        auto signedResultIndex = (step.offsetDeterminer > 0) ? startNodeRoughIndex + (step.offsetDeterminer - 1)
                                                             : startNodeRoughIndex + step.offsetDeterminer;
        if (signedResultIndex < 0 || signedResultIndex >= static_cast<decltype(signedResultIndex)>(candidates.size())) {
            // no matches
            isGood = false;
            return currentNodeIndex;
        }
        isGood = true;
        return candidates.at(static_cast<std::size_t>(signedResultIndex));
    }

    if (step.indexDeterminer != -1) {
        if (step.indexDeterminer >= static_cast<int>(candidates.size())) {
            isGood = false;
            return currentNodeIndex;
        }
        isGood = true;
        return candidates.at(static_cast<std::size_t>(step.indexDeterminer));
    }

    // no determiner
    isGood = (candidates.size() == 1);
    return candidates.front();
}

int Tree::nodeTraverse(int currentNodeIndex, const Node& localValueEvaluationNode, const QVector<NodeTraverseStep>& steps, bool& isGood) const
{
    for (const auto& step : steps) {
        currentNodeIndex = nodeTraverse(currentNodeIndex, localValueEvaluationNode, step, isGood);
        if (!isGood)
            break;
    }
    return currentNodeIndex;
}

int Tree::nodeTraverse(const QVector<NodeTraverseStep>& steps, bool& isGood, const EvaluationContext& ctx, int treeIndex)
{
    const Node& localvalueEvaluationNode = ctx.mainTree.getNode(ctx.startNodeIndex);
    const Tree& tree = (treeIndex == -1)? ctx.mainTree : *ctx.sideTreeList.at(treeIndex);
    int currentNodeIndex = (treeIndex == -1)? ctx.startNodeIndex : 0;
    return tree.nodeTraverse(currentNodeIndex, localvalueEvaluationNode, steps, isGood);
}

QString Tree::evaluateSingleValueExpression(const SingleValueExpression& expr, bool &isGood, const EvaluationContext& ctx)
{
    const Node& localvalueEvaluationNode = ctx.mainTree.getNode(ctx.startNodeIndex);
    const Tree& tree = (expr.treeIndex == -1)? ctx.mainTree : *ctx.sideTreeList.at(expr.treeIndex);
    if (!expr.traversal.isEmpty()) {
        // we need to do a traverse
        Q_ASSERT(expr.es != SingleValueExpression::EvaluateStrategy::DefaultOnly);

        int currentNodeIndex = nodeTraverse(expr.traversal, isGood, ctx, expr.treeIndex);
        if (!isGood && expr.es == SingleValueExpression::EvaluateStrategy::TraverseOnly) {
            return QString();
        }
        QString result = evaluateLocalValueExpression(tree.getNode(currentNodeIndex), expr.exprAtDestinationNode, isGood);
        if (isGood) {
            return result;
        } else if (expr.es == SingleValueExpression::EvaluateStrategy::TraverseOnly) {
            return QString();
        }
    }

    switch (expr.es) {
    case SingleValueExpression::EvaluateStrategy::TraverseOnly: {
        // suppress compiler warnings
        qFatal("Impossible");
        return QString();
    }
    case SingleValueExpression::EvaluateStrategy::DefaultOnly:
    case SingleValueExpression::EvaluateStrategy::TraverseWithFallback: {
        return evaluateLocalValueExpression(localvalueEvaluationNode, expr.defaultValue, isGood);
    }
    }
    qFatal("Impossible");
    return QString();
}

bool Tree::evaluatePredicate(const Predicate& pred, bool& isGood, const EvaluationContext &ctx)
{
    bool result = false;
    switch (pred.ty) {
    case Predicate::PredicateType::ValueEqual: {
        QString lhs = evaluateSingleValueExpression(pred.v1, isGood, ctx);
        if (!isGood)
            return false;
        QString rhs = evaluateSingleValueExpression(pred.v2, isGood, ctx);
        if (!isGood)
            return false;
        result = (lhs == rhs);
    }break;
    case Predicate::PredicateType::NodeExist: {
        int node = nodeTraverse(pred.nodeTest.steps, result, ctx, pred.nodeTest.treeIndex);
        Q_UNUSED(node)
    }break;
    }
    return (pred.isInvert? !result : result);
}

QString Tree::evaluateGeneralValueExpression(const GeneralValueExpression& expr, bool& isGood, const EvaluationContext& ctx)
{
    for (const auto& p : expr.branches) {
        const auto& predicates = p.predicates;
        bool isFailed = false;
        for (const auto& pred : predicates) {
            bool isPredGood = false;
            if (!evaluatePredicate(pred, isPredGood, ctx) || !isPredGood) {
                isFailed = true;
                break;
            }
        }
        if (isFailed)
            continue;

        QString result = evaluateSingleValueExpression(p.value, isGood, ctx);
        if (!isGood)
            continue;

        return result;
    }

    // no matches
    isGood = false;
    return QString();
}

// ----------------------------------------------------------------------------
namespace {
const QString XML_LV_TYPE = QStringLiteral("LocalValueType");
const QString XML_LV_TYPE_LITERAL = QStringLiteral("Literal");
const QString XML_LV_TYPE_NODETYPE = QStringLiteral("NodeType");
const QString XML_LV_TYPE_KV = QStringLiteral("KeyValue");
} // end of anonymous namespace

void Tree::LocalValueExpression::saveToXML(QXmlStreamWriter& xml) const
{
    switch (ty) {
    case Tree::LocalValueExpression::ValueType::Literal: {
        xml.writeAttribute(XML_LV_TYPE, XML_LV_TYPE_LITERAL);
    }break;
    case Tree::LocalValueExpression::ValueType::KeyValue: {
        xml.writeAttribute(XML_LV_TYPE, XML_LV_TYPE_KV);
    }break;
    case Tree::LocalValueExpression::ValueType::NodeType: {
        xml.writeAttribute(XML_LV_TYPE, XML_LV_TYPE_NODETYPE);
    }break;
    }
    if (ty != Tree::LocalValueExpression::ValueType::NodeType) {
        xml.writeCharacters(str);
    }
    xml.writeEndElement();
}

bool Tree::LocalValueExpression::loadFromXML(QXmlStreamReader& xml, StringCache &strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    {
        auto attr = xml.attributes();
        if (Q_UNLIKELY(!attr.hasAttribute(XML_LV_TYPE))) {
            qWarning() << "Missing " << XML_LV_TYPE << "attribute on " << xml.name() << "element";
            return false;
        }
        auto tyFromXML = attr.value(XML_LV_TYPE);
        if (tyFromXML == XML_LV_TYPE_LITERAL) {
            ty = ValueType::Literal;
        } else if (tyFromXML == XML_LV_TYPE_KV) {
            ty = ValueType::KeyValue;
        } else if (tyFromXML == XML_LV_TYPE_NODETYPE) {
            ty = ValueType::NodeType;
        } else {
            qWarning() << "Unknown value type " << tyFromXML << ", expecting one of the following: {"
                       << XML_LV_TYPE_LITERAL << ", " << XML_LV_TYPE_KV << ", " << XML_LV_TYPE_NODETYPE << "}";
            return false;
        }
    }

    if (ty != ValueType::NodeType) {
        // we need to read a string that's either the literal or the key for key value pair
        if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
            auto msg = qWarning();
            switch (ty) {
            case ValueType::NodeType: Q_UNREACHABLE();
            case ValueType::KeyValue:
                msg << "When reading key for KeyValue LocalValueExpr: ";
                break;
            case ValueType::Literal:
                msg << "When reading string for Literal LocalValueExpr: ";
                break;
            }
            msg << "Unexpected xml token " << xml.tokenString() << "(Expecting Characters)";
            return false;
        }
        str = strCache(xml.text());

    } else {
        str.clear();
    }

    xml.skipCurrentElement();
    return true;
}

namespace  {
const QString XML_STEP = QStringLiteral("Step");
const QString XML_STEP_DESTINATION = QStringLiteral("StepDestination");
const QString XML_STEP_DEST_PARENT = QStringLiteral("Parent");
const QString XML_STEP_DEST_CHILD = QStringLiteral("Child");
const QString XML_STEP_DEST_PEER = QStringLiteral("Peer");
const QString XML_STEP_TYPEFILTER = QStringLiteral("NodeTypeFilter");
const QString XML_STEP_KEYVALUEFILTER = QStringLiteral("KeyValueFilter");
const QString XML_KEY = QStringLiteral("Key");
const QString XML_VALUE = QStringLiteral("Value");
const QString XML_TREEINDEX = QStringLiteral("TreeIndex");
} // end of anonymous namespace

void Tree::NodeTraverseStep::saveToXML(QXmlStreamWriter& xml) const
{
    switch (destination) {
    case StepDestination::Parent: {
        xml.writeAttribute(XML_STEP_DESTINATION, XML_STEP_DEST_PARENT);
    }break;
    case StepDestination::Peer: {
        xml.writeAttribute(XML_STEP_DESTINATION, XML_STEP_DEST_PEER);
    }break;
    case StepDestination::Child: {
        xml.writeAttribute(XML_STEP_DESTINATION, XML_STEP_DEST_CHILD);
    }break;
    }
    if (destination != StepDestination::Parent) {
        // need to save filter info
        if (!childTypeFilter.isEmpty()) {
            xml.writeTextElement(XML_STEP_TYPEFILTER, childTypeFilter);
        }
        for (auto iter = keyValueFilter.begin(), iterEnd = keyValueFilter.end(); iter != iterEnd; ++iter) {
            xml.writeStartElement(XML_STEP_KEYVALUEFILTER);
            xml.writeTextElement(XML_KEY, iter.key());
            {
                xml.writeStartElement(XML_VALUE);
                iter.value().saveToXML(xml);
            }
            xml.writeEndElement();
        }
    }
    xml.writeEndElement();
}

bool Tree::NodeTraverseStep::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    {
        auto attr = xml.attributes();
        if (Q_UNLIKELY(!attr.hasAttribute(XML_STEP_DESTINATION))) {
            qWarning() << "Missing " << XML_STEP_DESTINATION << "attribute on " << xml.name() << "element";
            return false;
        }
        auto destFromXML = attr.value(XML_STEP_DESTINATION);
        if (destFromXML == XML_STEP_DEST_PARENT) {
            destination = StepDestination::Parent;
        } else if (destFromXML == XML_STEP_DEST_PEER) {
            destination = StepDestination::Peer;
        } else if (destFromXML == XML_STEP_DEST_CHILD) {
            destination = StepDestination::Child;
        } else {
            qWarning() << "Unknown destination type " << destFromXML << ", expecting one of the following: {"
                       << XML_STEP_DEST_PARENT << ", " << XML_STEP_DEST_PEER << ", " << XML_STEP_DEST_CHILD << "}";
            return false;
        }
    }
    childTypeFilter.clear();
    keyValueFilter.clear();
    while (xml.readNextStartElement()) {
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
        if (xml.name() == XML_STEP_KEYVALUEFILTER) {
            while (xml.readNext() != QXmlStreamReader::StartElement) {
                if (Q_LIKELY(xml.tokenType() == QXmlStreamReader::Comment || xml.tokenType() == QXmlStreamReader::Characters))
                    continue;
                qWarning() << "When reading" << XML_STEP_KEYVALUEFILTER <<": unexpected xml token" << xml.tokenString() << "(Expecting StartElement for key)";
                return false;
            }
            if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
                qWarning() << "When reading" << XML_STEP_KEYVALUEFILTER << ": unexpected xml token " << xml.tokenString() << "(Expecting Characters for value)";
                return false;
            }
            QString key = strCache(xml.text());
            if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
                qWarning() << "When wrapping up key inside KeyValueFilter: unexpected xml token " << xml.tokenString() << "(Expecting EndElement)";
                return false;
            }
            while (xml.readNext() != QXmlStreamReader::StartElement) {
                if (Q_LIKELY(xml.tokenType() == QXmlStreamReader::Comment || xml.tokenType() == QXmlStreamReader::Characters))
                    continue;
                qWarning() << "When reading" << XML_STEP_KEYVALUEFILTER <<": unexpected xml token" << xml.tokenString() << "(Expecting StartElement for value)";
                return false;
            }
            LocalValueExpression value;
            if (Q_UNLIKELY(!value.loadFromXML(xml, strCache))) {
                qWarning() << "When reading" << XML_STEP_KEYVALUEFILTER <<": failed to read value";
                return false;
            }
            keyValueFilter.insert(key, value);
            if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
                qWarning() << "When wrapping up" << XML_STEP_KEYVALUEFILTER << ": unexpected xml token " << xml.tokenString() << "(Expecting EndElement)";
                return false;
            }
        } else if (xml.name() == XML_STEP_TYPEFILTER) {
            if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
                qWarning() << "When reading" << XML_STEP_TYPEFILTER << ": unexpected xml token " << xml.tokenString() << "(Expecting Characters)";
                return false;
            }
            if (Q_UNLIKELY(!childTypeFilter.isEmpty())) {
                qWarning() << "Multiple node type filter in NodeTraverseStep; previous value" << childTypeFilter << "is discarded";
            }
            childTypeFilter = strCache(xml.text());
            if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
                qWarning() << "When wrapping up" << XML_STEP_TYPEFILTER << ": unexpected xml token " << xml.tokenString() << "(Expecting EndElement)";
                return false;
            }
        } else {
            qWarning() << "Unexpected element" << xml.name() << "under NodeTraverseStep";
            return false;
        }
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        qWarning() << "When wrapping up NodeTraverseStep: unexpected xml token " << xml.tokenString() << "(Expecting EndElement)";
        return false;
    }
    return true;
}

void Tree::NodeTraverseStep::saveToXML(QXmlStreamWriter& xml, const QVector<NodeTraverseStep>& steps)
{
    for (const auto& step : steps) {
        xml.writeStartElement(XML_STEP);
        step.saveToXML(xml);
    }
    xml.writeEndElement();
}

void Tree::NodeTraverseStep::saveToXML(QXmlStreamWriter& xml, const QVector<NodeTraverseStep>& steps, int treeIndex)
{
    if (treeIndex >= 0) {
        xml.writeAttribute(XML_TREEINDEX, QString::number(treeIndex));
    }
    saveToXML(xml, steps);
}

bool Tree::NodeTraverseStep::loadFromXML(QXmlStreamReader& xml, StringCache& strCache, QVector<NodeTraverseStep>& steps)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    steps.clear();
    while (xml.readNextStartElement()) {
        if (Q_UNLIKELY(xml.name() != XML_STEP)) {
            qWarning() << "Unexpected xml element" << xml.name() << "(Expecting " << XML_STEP <<")";
            return false;
        }
        steps.push_back(NodeTraverseStep());
        if (Q_UNLIKELY(!steps.back().loadFromXML(xml, strCache))) {
            return false;
        }
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        qWarning() << "When wrapping up NodeTraverseStep: unexpected xml token " << xml.tokenString() << "(Expecting EndElement)";
        return false;
    }
    return true;
}

bool Tree::NodeTraverseStep::loadFromXML(QXmlStreamReader& xml, StringCache& strCache, QVector<NodeTraverseStep>& steps, int& treeIndex)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    treeIndex = -1;
    auto attr = xml.attributes();
    if (attr.hasAttribute(XML_TREEINDEX)) {
        auto idx = attr.value(XML_TREEINDEX);
        bool isGood = false;
        treeIndex = idx.toInt(&isGood);
        if (Q_UNLIKELY(!isGood)) {
            qWarning() << "Invalid tree index " << idx;
            return false;
        }
    }
    return loadFromXML(xml, strCache, steps);
}

namespace  {
const QString XML_SV_TYPE = QStringLiteral("SingleValueType");
const QString XML_SV_LV = QStringLiteral("LocalValue");
const QString XML_SV_TRAVERSE_ONLY = QStringLiteral("RemoteValue");
const QString XML_SV_FULL = QStringLiteral("RemoteValueWithFallback");
const QString XML_SV_DEFAULT = QStringLiteral("DefaultLocalValue");
const QString XML_SV_PATH = QStringLiteral("Path");
const QString XML_SV_VALUE_AT_DEST = QStringLiteral("LocalValueAtDestination");
} // end of anonymous namespace

void Tree::SingleValueExpression::saveToXML(QXmlStreamWriter& xml) const
{
    switch (es) {
    case EvaluateStrategy::DefaultOnly: {
        xml.writeAttribute(XML_SV_TYPE, XML_SV_LV);
        defaultValue.saveToXML(xml);
        return;
    }
    case EvaluateStrategy::TraverseOnly: {
        xml.writeAttribute(XML_SV_TYPE, XML_SV_TRAVERSE_ONLY);
    }break;
    case EvaluateStrategy::TraverseWithFallback: {
        xml.writeAttribute(XML_SV_TYPE, XML_SV_FULL);
    }break;
    }

    xml.writeStartElement(XML_SV_PATH);
    NodeTraverseStep::saveToXML(xml, traversal, treeIndex);
    xml.writeStartElement(XML_SV_VALUE_AT_DEST);
    exprAtDestinationNode.saveToXML(xml);

    if (es == EvaluateStrategy::TraverseWithFallback) {
        xml.writeStartElement(XML_SV_DEFAULT);
        defaultValue.saveToXML(xml);
    }
}

bool Tree::SingleValueExpression::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    treeIndex = -1;
    traversal.clear();
    {
        auto attr = xml.attributes();
        if (Q_UNLIKELY(!attr.hasAttribute(XML_SV_TYPE))) {
            qWarning() << "Missing " << XML_SV_TYPE << "attribute on " << xml.name() << "element";
            return false;
        }
        auto tyFromXML = attr.value(XML_SV_TYPE);
        if (tyFromXML == XML_SV_LV) {
            es = EvaluateStrategy::DefaultOnly;
            // in this case no traverse path would be needed
            return defaultValue.loadFromXML(xml, strCache);
        } else if (tyFromXML == XML_SV_TRAVERSE_ONLY) {
            es = EvaluateStrategy::TraverseOnly;
        } else if (tyFromXML == XML_SV_FULL) {
            es = EvaluateStrategy::TraverseWithFallback;
        } else {
            qWarning() << "Unknown ValueType " << tyFromXML << ", expecting one of the following: {"
                       << XML_SV_LV << ", " << XML_SV_TRAVERSE_ONLY << ", " << XML_SV_FULL << "}";
            return false;
        }
    }

    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "When reading SingleValueExpression: StartElement for" << XML_SV_PATH << "is not found";
        return false;
    }
    if (Q_UNLIKELY(xml.name() != XML_SV_PATH)) {
        qWarning() << "When reading SingleValueExpression: unexpected element" << xml.name() <<"(Expecting" << XML_SV_PATH << ")";
        return false;
    }
    if (Q_UNLIKELY(!NodeTraverseStep::loadFromXML(xml, strCache, traversal, treeIndex))) {
        return false;
    }
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "When reading SingleValueExpression: StartElement for" << XML_SV_VALUE_AT_DEST << "is not found";
        return false;
    }
    if (Q_UNLIKELY(xml.name() != XML_SV_VALUE_AT_DEST)) {
        qWarning() << "When reading SingleValueExpression: unexpected element" << xml.name() <<"(Expecting" << XML_SV_VALUE_AT_DEST << ")";
        return false;
    }
    if (Q_UNLIKELY(!exprAtDestinationNode.loadFromXML(xml, strCache))) {
        return false;
    }
    if (es == EvaluateStrategy::TraverseWithFallback) {
        if (Q_UNLIKELY(!xml.readNextStartElement())) {
            qWarning() << "When reading SingleValueExpression: StartElement for" << XML_SV_DEFAULT << "is not found";
            return false;
        }
        if (Q_UNLIKELY(xml.name() != XML_SV_DEFAULT)) {
            qWarning() << "When reading SingleValueExpression: unexpected element" << xml.name() <<"(Expecting" << XML_SV_DEFAULT << ")";
            return false;
        }
        if (Q_UNLIKELY(!defaultValue.loadFromXML(xml, strCache))) {
            return false;
        }
    }
    xml.skipCurrentElement();
    return true;
}

namespace  {
const QString XML_PRED = QStringLiteral("Predicate");
const QString XML_PRED_TYPE = QStringLiteral("PredicateType");
const QString XML_PRED_TYPE_VALUE_EQUAL = QStringLiteral("ValueEqual");
const QString XML_PRED_TYPE_NODE_EXIST = QStringLiteral("NodeExist");
const QString XML_PRED_INVERT = QStringLiteral("InvertResult");
const QString XML_YES = QStringLiteral("Yes");
const QString XML_NO = QStringLiteral("No");
const QString XML_PRED_PATH = QStringLiteral("Path");
const QString XML_PRED_VALUE1 = QStringLiteral("Value1");
const QString XML_PRED_VALUE2 = QStringLiteral("Value2");
} // end of anonymous namespace

void Tree::Predicate::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeAttribute(XML_PRED_INVERT, (isInvert ? XML_YES : XML_NO));
    switch (ty) {
    case PredicateType::ValueEqual: {
        xml.writeAttribute(XML_PRED_TYPE, XML_PRED_TYPE_VALUE_EQUAL);
        xml.writeStartElement(XML_PRED_VALUE1);
        v1.saveToXML(xml);
        xml.writeStartElement(XML_PRED_VALUE2);
        v2.saveToXML(xml);
    }break;
    case PredicateType::NodeExist: {
        xml.writeAttribute(XML_PRED_TYPE, XML_PRED_TYPE_NODE_EXIST);
        xml.writeStartElement(XML_PRED_PATH);
        NodeTraverseStep::saveToXML(xml, nodeTest.steps, nodeTest.treeIndex);
    }break;
    }
    xml.writeEndElement();
}

bool Tree::Predicate::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    {
        auto attr = xml.attributes();
        {
            if (Q_UNLIKELY(!attr.hasAttribute(XML_PRED_INVERT))) {
                qWarning() << "Missing " << XML_PRED_INVERT << "attribute on " << xml.name() << "element";
                return false;
            }
            auto val = attr.value(XML_PRED_INVERT);
            if (val == XML_YES) {
                isInvert = true;
            } else if (val == XML_NO) {
                isInvert = false;
            } else {
                qWarning() << "Unknown" << XML_PRED_INVERT << "value:" << val << ", expecting one of the following: {"
                           << XML_YES << ", " << XML_NO << "}";
                return false;
            }
        }
        {
            if (Q_UNLIKELY(!attr.hasAttribute(XML_PRED_TYPE))) {
                qWarning() << "Missing " << XML_PRED_TYPE << "attribute on " << xml.name() << "element";
                return false;
            }
            auto val = attr.value(XML_PRED_TYPE);
            if (val == XML_PRED_TYPE_VALUE_EQUAL) {
                ty = PredicateType::ValueEqual;
            } else if (val == XML_PRED_TYPE_NODE_EXIST) {
                ty = PredicateType::NodeExist;
            } else {
                qWarning() << "Unknown" << XML_PRED_TYPE << "value:" << val << ", expecting one of the following: {"
                           << XML_PRED_TYPE_VALUE_EQUAL << ", " << XML_PRED_TYPE_NODE_EXIST << "}";
                return false;
            }
        }
    }
    switch (ty) {
    case PredicateType::ValueEqual: {
        if (Q_UNLIKELY(!xml.readNextStartElement())) {
            qWarning() << "When reading Predicate: StartElement for" << XML_PRED_VALUE1 << "is not found";
            return false;
        }
        if (Q_UNLIKELY(xml.name() != XML_PRED_VALUE1)) {
            qWarning() << "When reading Predicate: unexpected element" << xml.name() <<"(Expecting" << XML_PRED_VALUE1 << ")";
            return false;
        }
        if (Q_UNLIKELY(!v1.loadFromXML(xml, strCache))) {
            return false;
        }
        if (Q_UNLIKELY(!xml.readNextStartElement())) {
            qWarning() << "When reading Predicate: StartElement for" << XML_PRED_VALUE2 << "is not found";
            return false;
        }
        if (Q_UNLIKELY(xml.name() != XML_PRED_VALUE2)) {
            qWarning() << "When reading Predicate: unexpected element" << xml.name() <<"(Expecting" << XML_PRED_VALUE2 << ")";
            return false;
        }
        if (Q_UNLIKELY(!v2.loadFromXML(xml, strCache))) {
            return false;
        }
    }break;
    case PredicateType::NodeExist: {
        if (Q_UNLIKELY(!xml.readNextStartElement())) {
            qWarning() << "When reading Predicate: StartElement for" << XML_PRED_PATH << "is not found";
            return false;
        }
        if (Q_UNLIKELY(xml.name() != XML_PRED_PATH)) {
            qWarning() << "When reading Predicate: unexpected element" << xml.name() <<"(Expecting" << XML_PRED_PATH << ")";
            return false;
        }
        if (Q_UNLIKELY(!NodeTraverseStep::loadFromXML(xml, strCache, nodeTest.steps, nodeTest.treeIndex))) {
            return false;
        }
    }break;
    }
    xml.skipCurrentElement();
    return true;
}

void Tree::Predicate::saveToXML(QXmlStreamWriter& xml, const QVector<Predicate>& predicates)
{
    for (const auto& pred : predicates) {
        xml.writeStartElement(XML_PRED);
        pred.saveToXML(xml);
    }
    xml.writeEndElement();
}

bool Tree::Predicate::loadFromXML(QXmlStreamReader& xml, StringCache& strCache, QVector<Predicate>& predicates)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    predicates.clear();
    while (xml.readNextStartElement()) {
        if (Q_UNLIKELY(xml.name() != XML_PRED)) {
            qWarning() << "Unexpected xml element" << xml.name() << "(Expecting " << XML_PRED <<")";
            return false;
        }
        predicates.push_back(Predicate());
        if (Q_UNLIKELY(!predicates.back().loadFromXML(xml, strCache))) {
            return false;
        }
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        qWarning() << "When wrapping up predicate list: unexpected xml token " << xml.tokenString() << "(Expecting EndElement)";
        return false;
    }
    return true;
}

namespace  {
const QString XML_BRANCH_NOPRED = QStringLiteral("BranchNoPredicate");
const QString XML_BRANCH_PREDICATES = QStringLiteral("Predicates");
} // end of anonymous namespace

void Tree::BranchedValueExpression::Branch::saveToXML(QXmlStreamWriter& xml) const
{
    if (predicates.isEmpty()) {
        xml.writeAttribute(XML_BRANCH_NOPRED, XML_YES);
        value.saveToXML(xml);
        return;
    }
    // we have predicates
    xml.writeStartElement(XML_BRANCH_PREDICATES);
    Predicate::saveToXML(xml, predicates);
    xml.writeStartElement(XML_VALUE);
    value.saveToXML(xml);
    return;
}

bool Tree::BranchedValueExpression::Branch::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    predicates.clear();
    {
        auto attr = xml.attributes();
        if (attr.hasAttribute(XML_BRANCH_NOPRED)) {
            auto val = attr.value(XML_BRANCH_NOPRED);
            if (Q_UNLIKELY(val != XML_YES)) {
                qWarning() << "When reading Branch: unknown " << XML_BRANCH_NOPRED << "value " << val;
                return false;
            }
            return value.loadFromXML(xml, strCache);
        }
    }

    // okay, this branch has predicates
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "When reading Branch: StartElement for" << XML_BRANCH_PREDICATES << "is not found";
        return false;
    }
    if (Q_UNLIKELY(xml.name() != XML_BRANCH_PREDICATES)) {
        qWarning() << "When reading Branch: unexpected element" << xml.name() <<"(Expecting" << XML_BRANCH_PREDICATES << ")";
        return false;
    }
    if (Q_UNLIKELY(!Predicate::loadFromXML(xml, strCache, predicates))) {
        return false;
    }
    // now the value
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        qWarning() << "When reading Branch: StartElement for" << XML_VALUE << "is not found";
        return false;
    }
    if (Q_UNLIKELY(xml.name() != XML_VALUE)) {
        qWarning() << "When reading Branch: unexpected element" << xml.name() <<"(Expecting" << XML_VALUE << ")";
        return false;
    }
    if (Q_UNLIKELY(!value.loadFromXML(xml, strCache))) {
        return false;
    }

    xml.skipCurrentElement();
    return true;
}

namespace  {
const QString XML_BV_SINGLEBRANCH = QStringLiteral("BranchedValueSingleBranch");
const QString XML_BRANCH = QStringLiteral("Branch");
} // end of anonymous namespace

void Tree::BranchedValueExpression::saveToXML(QXmlStreamWriter& xml) const
{
    if (branches.size() == 1) {
        xml.writeAttribute(XML_BV_SINGLEBRANCH, XML_YES);
        branches.front().saveToXML(xml);
        return;
    }
    for (const auto& branch : branches) {
        xml.writeStartElement(XML_BRANCH);
        branch.saveToXML(xml);
    }
    xml.writeEndElement();
}

bool Tree::BranchedValueExpression::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    branches.clear();
    {
        auto attr = xml.attributes();
        if (attr.hasAttribute(XML_BV_SINGLEBRANCH)) {
            auto val = attr.value(XML_BV_SINGLEBRANCH);
            if (Q_UNLIKELY(val != XML_YES)) {
                qWarning() << "When reading BranchedValue: unknown " << XML_BV_SINGLEBRANCH << "value " << val;
                return false;
            }
            branches.push_back(Branch());
            return branches.back().loadFromXML(xml, strCache);
        }
    }
    while (xml.readNextStartElement()) {
        if (Q_UNLIKELY(xml.name() != XML_BRANCH)) {
            qWarning() << "Unexpected xml element" << xml.name() << "(Expecting " << XML_BRANCH <<")";
            return false;
        }
        branches.push_back(Branch());
        if (Q_UNLIKELY(!branches.back().loadFromXML(xml, strCache))) {
            return false;
        }
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        qWarning() << "When wrapping up BranchedValue: unexpected xml token " << xml.tokenString() << "(Expecting EndElement)";
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------

Tree::Tree(const TreeBuilder &tree)
{
    Q_ASSERT(tree.root != nullptr);
    QVector<int> dummy;
    TreeBuilder::populateNodeList(nodes, dummy, nullptr, 0, tree.root);
    Q_ASSERT(dummy.size() == 1);
}

Tree::Tree(const TreeBuilder& tree, QVector<int>& sequenceNumberTable)
{
    Q_ASSERT(tree.root != nullptr);
    QVector<int> dummy;
    TreeBuilder::populateNodeList(nodes, dummy, &sequenceNumberTable, 0, tree.root);
    Q_ASSERT(dummy.size() == 1);
}

void TreeBuilder::populateNodeList(QVector<Tree::Node>& nodes, QVector<int>& childTable, QVector<int> *sequenceNumberTable, int parentIndex, TreeBuilder::Node* subtreeRoot)
{
    Q_ASSERT(subtreeRoot);
    int nodeIndex = nodes.size();
    {
        Tree::Node curNode;
        int offset = nodeIndex - parentIndex;
        curNode.offsetFromParent = offset;
        curNode.typeName = subtreeRoot->typeName;
        curNode.keyList = subtreeRoot->keyList;
        curNode.valueList = subtreeRoot->valueList;
        nodes.push_back(curNode);
        if (sequenceNumberTable) {
            sequenceNumberTable->push_back(subtreeRoot->sequenceNumber);
        }
        childTable.push_back(offset);
    }
    QVector<int> childOffsets;
    for (TreeBuilder::Node* child = subtreeRoot->childStart; child != nullptr; child = child->nextPeer) {
        populateNodeList(nodes, childOffsets, sequenceNumberTable, nodeIndex, child);
    }
    nodes[nodeIndex].offsetToChildren.swap(childOffsets);
}

void TreeBuilder::Node::detach()
{
    // if the node is already detached, leave it there
    if (parent == nullptr) {
        Q_ASSERT(previousPeer == nullptr && nextPeer == nullptr);
        return;
    }

    if (previousPeer) {
        Q_ASSERT(previousPeer->nextPeer == this);
        previousPeer->nextPeer = nextPeer;
    } else {
        Q_ASSERT(parent->childStart == this);
        parent->childStart = nextPeer;
    }

    if (nextPeer) {
        Q_ASSERT(nextPeer->previousPeer == this);
        nextPeer->previousPeer = previousPeer;
    }

    parent = nullptr;
    previousPeer = nullptr;
    nextPeer = nullptr;
}

void TreeBuilder::Node::setParent(Node* newParent)
{
    Q_ASSERT(newParent);

    detach();
    parent = newParent;
    if (newParent->childStart == nullptr) {
        newParent->childStart = this;
        return;
    }
    Node* predecessor = newParent->childStart;
    while (predecessor->nextPeer != nullptr) {
        predecessor = predecessor->nextPeer;
    }
    predecessor->nextPeer = this;
    previousPeer = predecessor;
}

void TreeBuilder::Node::changePosition(Node* newParent, Node* newPredecessor)
{
    if (newParent == nullptr)
        newParent = parent;

    Q_ASSERT(newParent);

    detach();
    parent = newParent;
    if (newParent->childStart == nullptr) {
        Q_ASSERT(newPredecessor == nullptr);
        newParent->childStart = this;
        return;
    }

    Node* predecessor = newParent->childStart;
    while (predecessor != newPredecessor) {
        predecessor = predecessor->nextPeer;
        Q_ASSERT(predecessor);
    }
    predecessor->nextPeer = this;
    previousPeer = predecessor;
}

void TreeBuilder::clear(){
    for (auto ptr : nodes) {
        if (ptr)
            delete ptr;
    }
    sequenceCounter = 0;
    root = nullptr;
}

TreeBuilder::Node* TreeBuilder::allocateNode()
{
    Node* ptr = new Node;
    ptr->sequenceNumber = sequenceCounter++;
    nodes.push_back(ptr);
    return ptr;
}

TreeBuilder::Node* TreeBuilder::addNode(Node* parent)
{
    Node* ptr = allocateNode();
    Q_ASSERT(ptr);
    if (parent) {
        ptr->setParent(parent);
    } else {
        if (!root)
            root = ptr;
    }
    return ptr;
}
