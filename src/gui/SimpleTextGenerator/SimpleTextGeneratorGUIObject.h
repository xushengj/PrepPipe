#ifndef SIMPLETEXTGENERATORGUIOBJECT_H
#define SIMPLETEXTGENERATORGUIOBJECT_H

#include "src/lib/TaskObject/SimpleTextGeneratorObject.h"

#include <QObject>

class SimpleTextGeneratorGUIObject final : public SimpleTextGeneratorObject
{
    Q_OBJECT
public:
    struct RuleGUIData {
        QStringList headerExampleText;
        QStringList delimiterExampleText;
        QStringList tailExampleText;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    SimpleTextGeneratorGUIObject();
    SimpleTextGeneratorGUIObject(const SimpleTextGenerator::Data& dataArg, const QHash<QString, RuleGUIData>& guiDataArg);
    SimpleTextGeneratorGUIObject(const SimpleTextGeneratorGUIObject& src) = default;
    virtual ~SimpleTextGeneratorGUIObject() override {}

    virtual SimpleTextGeneratorGUIObject* clone() override {
        return new SimpleTextGeneratorGUIObject(*this);
    }

    static SimpleTextGeneratorGUIObject* loadFromXML(QXmlStreamReader& xml, StringCache &strCache);

    virtual QWidget* getEditor() override;
    virtual bool editorNoUnsavedChanges(QWidget* editorArg) override;
    virtual void saveDataFromEditor(QWidget* editorArg) override;

    QStringList getHeaderExampleText(const QString& canonicalName) const;
    QStringList getDelimiterExampleText(const QString& canonicalName) const;
    QStringList getTailExampleText(const QString& canonicalName) const;

    void setHeaderExampleText(const QString& canonicalName, const QStringList& textList) {
        guidata[canonicalName].headerExampleText = textList;
    }
    void setDelimiterExampleText(const QString& canonicalName, const QStringList& textList) {
        guidata[canonicalName].delimiterExampleText = textList;
    }
    void setTailExampleText(const QString& canonicalName, const QStringList& textList) {
        guidata[canonicalName].tailExampleText = textList;
    }
protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

protected:
    QHash<QString, RuleGUIData> guidata;
};

template <>
struct IntrinsicObjectTrait<SimpleTextGeneratorObject>
{
    static SimpleTextGeneratorObject* loadFromXML(QXmlStreamReader& xml, StringCache& strCache) {
        return SimpleTextGeneratorGUIObject::loadFromXML(xml, strCache);
    }
};

#endif // SIMPLETEXTGENERATORGUIOBJECT_H
