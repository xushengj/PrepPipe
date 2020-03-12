#ifndef GENERALTREE_H
#define GENERALTREE_H

#include "src/lib/ObjectBase.h"
#include "src/lib/IntrinsicObject.h"
#include "src/gui/TextEditor.h"
#include "src/lib/Tree/Tree.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class GeneralTreeObject : public IntrinsicObject
{
    Q_OBJECT
public:
    using Node = Tree::Node;

    explicit GeneralTreeObject(const Tree& tree);
    GeneralTreeObject();
    GeneralTreeObject(const GeneralTreeObject& src) = default;
    virtual ~GeneralTreeObject() override;

    virtual GeneralTreeObject* clone() override {
        return new GeneralTreeObject(*this);
    }

    static GeneralTreeObject* loadFromXML(QXmlStreamReader& xml, StringCache &strCache);

    const Tree& getTreeData() const {return treeData;}

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

private:
    void saveToXML(QXmlStreamWriter& xml, int nodeIndex); // recursive

private:
    Tree treeData;
};

#endif // GENERALTREE_H
