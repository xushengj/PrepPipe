#ifndef GENERALTREE_H
#define GENERALTREE_H

#include "src/lib/ObjectBase.h"
#include "src/lib/IntrinsicObject.h"
#include "src/gui/TextEditor.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class GeneralTree : public IntrinsicObject
{
    Q_OBJECT
public:
    struct Node {
        QString typeName;
        QStringList keyList;
        QStringList valueList;

        int parentIndex;
        QList<int> children;
    };
    GeneralTree(const ConstructOptions& opt);
    GeneralTree(const QList<Node>& _nodes, const ConstructOptions& opt);
    virtual ~GeneralTree() override;

    const Node& getNode(int index) const {return nodes.at(index);}

    virtual TextEditor* getEditor() override;

    static GeneralTree* loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt);

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

private:
    void saveToXML(QXmlStreamWriter& xml, int nodeIndex); // recursive

private:
    QList<Node> nodes;
    TextEditor* ui = nullptr;
};

#endif // GENERALTREE_H
