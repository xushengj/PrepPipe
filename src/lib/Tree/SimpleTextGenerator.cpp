#include "SimpleTextGenerator.h"

#include <QCoreApplication>

// ----------------------------------------------------------------------------
// TextGenerator implementation

SimpleTextGenerator::SimpleTextGenerator(const Data &d)
    : data(d)
{
    for (auto iter = data.nameAliases.begin(), iterEnd = data.nameAliases.end(); iter != iterEnd; ++iter) {
        QString canonicalName = iter.key();
        for (const auto& alias : iter.value()) {
            aliasToCanonicalNameMap.insert(alias, canonicalName);
        }
    }
}

bool SimpleTextGenerator::generationImpl(const Tree& src, QString& dest, int nodeIndex) const
{
    const Tree::Node& node = src.getNode(nodeIndex);
    QString canonicalName = aliasToCanonicalNameMap.value(node.typeName, node.typeName);
    auto iter = data.expansions.find(canonicalName);
    if (iter != data.expansions.end()) {
        // we find the rule
        const NodeExpansionRule& rule = iter.value();
        if (!writeFragment(dest, node, rule.header, data.evalFailPolicy)) {
            return false;
        }
        bool isFirst = true;
        for (int childOffset : node.offsetToChildren) {
            if (!isFirst) {
                if (!writeFragment(dest, node, rule.delimiter, data.evalFailPolicy)) {
                    return false;
                }
            } else {
                isFirst = false;
            }
            if (!generationImpl(src, dest, nodeIndex + childOffset)) {
                return false;
            }
        }
        if (!writeFragment(dest, node, rule.tail, data.evalFailPolicy)) {
            return false;
        }
    } else {
        // no rules found
        switch (data.unknownNodePolicy) {
        case UnknownNodePolicy::DefaultExpand: {
            for (int childOffset : node.offsetToChildren) {
                if (!generationImpl(src, dest, nodeIndex + childOffset)) {
                    return false;
                }
            }
            return true;
        }
        case UnknownNodePolicy::Ignore: {
            return true;
        }
        case UnknownNodePolicy::Error: {
            return false;
        }
        }
    }
    return true;
}

bool SimpleTextGenerator::writeFragment(QString& dest, const Tree::Node& node, const QVector<Tree::LocalValueExpression>& fragment, EvaluationFailPolicy failPolicy)
{
    for (int i = 0, n = fragment.size(); i < n; ++i) {
        const auto& expr = fragment.at(i);
        bool isGood = false;
        QString result = Tree::evaluateLocalValueExpression(node, expr, isGood);
        if (!isGood) {
            switch (failPolicy) {
            case EvaluationFailPolicy::SkipSubExpr: {

            }break;
            case EvaluationFailPolicy::Error: {
                return false;
            }
            }
        } else {
            dest.append(result);
        }
    }

    return true;
}

bool SimpleTextGenerator::performGeneration(const Tree& src, QString& dest) const
{
    dest.clear();
    return generationImpl(src, dest, 0);
}

bool SimpleTextGenerator::Data::validate(QString& err) const
{
    err.clear();
    // as long as there is no clash in aliases, it is fine
    QHash<QString, QString> registeredAliases;
    for (auto iter = nameAliases.begin(), iterEnd = nameAliases.end(); iter != iterEnd; ++iter) {
        QString canonicalName = iter.key();
        for (const auto& alias : iter.value()) {
            auto iterCur = registeredAliases.find(alias);
            if (Q_UNLIKELY(iterCur != registeredAliases.end())) {
                err = QCoreApplication::translate(
                            "SimpleTextGenerator",
                            "Multiple definition for alias \"%1\"; first for canonical name \"%2\" and second for \"%3\"")
                        .arg(alias, iterCur.value(), canonicalName);
                return false;
            } else {
                registeredAliases.insert(alias, canonicalName);
            }
        }
    }
    return true;
}

// ----------------------------------------------------------------------------
// XML Input / Output

namespace {
const QString XML_UNKNOWN_NODE_POLICY = QStringLiteral("UnknownNodePolicy");
const QString XML_EVALUATION_FAIL_POLICY = QStringLiteral("EvaluationFailPolicy");
const QString XML_ERROR = QStringLiteral("Error");
const QString XML_IGNORE = QStringLiteral("Ignore");
const QString XML_DEFAULT_EXPAND = QStringLiteral("DefaultExpand");
const QString XML_SKIP_SUBEXPR = QStringLiteral("SkipSubExpression");

const QString XML_NODE_EXPANSION_RULE_LIST = QStringLiteral("ModeExpansionRules");
const QString XML_NODE_EXPANSION_RULE = QStringLiteral("Rule");
const QString XML_CANONICAL_NAME = QStringLiteral("CanonicalName");

const QString XML_NAME_ALIAS_LIST = QStringLiteral("NodeTypeNameAliases");
const QString XML_NAME_ALIAS = QStringLiteral("AliasRecord");
const QString XML_ALIAS_ENTRY = QStringLiteral("Alias");

const QString XML_HEADER = QStringLiteral("Header");
const QString XML_DELIMITER = QStringLiteral("Delimiter");
const QString XML_TAIL = QStringLiteral("Tail");
const QString XML_EXPR = QStringLiteral("Expr");
}

void SimpleTextGenerator::Data::saveToXML_NoTerminate(QXmlStreamWriter& xml) const
{
    switch (unknownNodePolicy) {
    case UnknownNodePolicy::Ignore: {
        xml.writeTextElement(XML_UNKNOWN_NODE_POLICY, XML_IGNORE);
    }break;
    case UnknownNodePolicy::DefaultExpand: {
        xml.writeTextElement(XML_UNKNOWN_NODE_POLICY, XML_DEFAULT_EXPAND);
    }break;
    case UnknownNodePolicy::Error: {
        xml.writeTextElement(XML_UNKNOWN_NODE_POLICY, XML_ERROR);
    }break;
    }
    switch (evalFailPolicy) {
    case EvaluationFailPolicy::SkipSubExpr: {
        xml.writeTextElement(XML_EVALUATION_FAIL_POLICY, XML_SKIP_SUBEXPR);
    }break;
    case EvaluationFailPolicy::Error: {
        xml.writeTextElement(XML_EVALUATION_FAIL_POLICY, XML_ERROR);
    }break;
    }
    XMLUtil::writeLoadableHash(xml, expansions, XML_NODE_EXPANSION_RULE_LIST, XML_NODE_EXPANSION_RULE, XML_CANONICAL_NAME);
    XMLUtil::writeStringListHash(xml, nameAliases, XML_NAME_ALIAS_LIST, XML_NAME_ALIAS, XML_CANONICAL_NAME, XML_ALIAS_ENTRY, true);
}

bool SimpleTextGenerator::Data::loadFromXML_NoTerminate(QXmlStreamReader& xml, StringCache& strCache)
{
    auto readUnknownNodePolicyCB = [this] (QStringRef text) -> bool {
        if (text == XML_IGNORE) {
            unknownNodePolicy = UnknownNodePolicy::Ignore;
        } else if (text == XML_DEFAULT_EXPAND) {
            unknownNodePolicy = UnknownNodePolicy::DefaultExpand;
        } else if (text == XML_ERROR) {
            unknownNodePolicy = UnknownNodePolicy::Error;
        } else {
            return false;
        }
        return true;
    };
    auto readEvaluationFailPolicyCB = [this] (QStringRef text) -> bool {
        if (text == XML_SKIP_SUBEXPR) {
            evalFailPolicy = EvaluationFailPolicy::SkipSubExpr;
        } else if (text == XML_ERROR) {
            evalFailPolicy = EvaluationFailPolicy::Error;
        } else {
            return false;
        }
        return true;
    };
    const char* curElement = "SimpleTreeWalkTextGenerator::Data";
    if (Q_UNLIKELY(!XMLUtil::readEnum(xml, curElement, XML_UNKNOWN_NODE_POLICY, readUnknownNodePolicyCB, {XML_IGNORE, XML_ERROR}))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readEnum(xml, curElement, XML_EVALUATION_FAIL_POLICY, readEvaluationFailPolicyCB, {XML_SKIP_SUBEXPR, XML_ERROR}))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableHash(xml, curElement, XML_NODE_EXPANSION_RULE_LIST, XML_NODE_EXPANSION_RULE, XML_CANONICAL_NAME, expansions, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringListHash(xml, curElement, XML_NAME_ALIAS_LIST, XML_NAME_ALIAS, XML_CANONICAL_NAME, XML_ALIAS_ENTRY, nameAliases, strCache))) {
        return false;
    }
    return true;
}

void SimpleTextGenerator::Data::saveToXML(QXmlStreamWriter& xml) const
{
    saveToXML_NoTerminate(xml);
    xml.writeEndElement();
}

bool SimpleTextGenerator::Data::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    if (Q_UNLIKELY(!loadFromXML_NoTerminate(xml, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleTextGenerator::NodeExpansionRule::saveToXML(QXmlStreamWriter& xml) const
{
    XMLUtil::writeLoadableList(xml, header, XML_HEADER, XML_EXPR);
    XMLUtil::writeLoadableList(xml, delimiter, XML_DELIMITER, XML_EXPR);
    XMLUtil::writeLoadableList(xml, tail, XML_TAIL, XML_EXPR);
    xml.writeEndElement();
}

bool SimpleTextGenerator::NodeExpansionRule::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleTreeWalkTextGenerator::NodeExpansionRule";
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_HEADER, XML_EXPR, header, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_DELIMITER, XML_EXPR, delimiter, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_TAIL, XML_EXPR, tail, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}
