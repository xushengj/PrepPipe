#ifndef SIMPLETEXTGENERATOR_H
#define SIMPLETEXTGENERATOR_H

#include <QString>
#include <QStringRef>
#include <QStringList>
#include <QList>
#include <QVector>
#include <QHash>
#include <QMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "src/lib/Tree/Tree.h"
#include "src/utils/XMLUtilities.h"

class SimpleTextGenerator
{

public:
    struct NodeExpansionRule {
        QVector<Tree::LocalValueExpression> header;
        QVector<Tree::LocalValueExpression> delimiter;
        QVector<Tree::LocalValueExpression> tail;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };
    enum class UnknownNodePolicy {
        Ignore,
        Error
    };
    enum class EvaluationFailPolicy {
        SkipSubExpr,
        Error
    };

    struct Data {
        UnknownNodePolicy unknownNodePolicy;
        EvaluationFailPolicy evalFailPolicy;
        QHash<QString, NodeExpansionRule> expansions; // canonical name -> rule
        QHash<QString, QStringList> nameAliases; // canonical name -> list of aliases

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);

        // the variant that do not produce / consume EndElement
        void saveToXML_NoTerminate(QXmlStreamWriter& xml) const;
        bool loadFromXML_NoTerminate(QXmlStreamReader& xml, StringCache& strCache);

        bool validate(QString& err) const;
    };

public:
    explicit SimpleTextGenerator(const Data& d);
    bool performGeneration(const Tree& src, QString& dest) const;
    static bool writeFragment(QString& dest, const Tree::Node& node, const QVector<Tree::LocalValueExpression> &fragment, EvaluationFailPolicy failPolicy);

private:
    bool generationImpl(const Tree& src, QString& dest, int nodeIndex) const;


private:
    // All persistent data should be in Data
    Data data;

private:
    // helper data
    QHash<QString, QString> aliasToCanonicalNameMap;
};

#endif // SIMPLETEXTGENERATOR_H
