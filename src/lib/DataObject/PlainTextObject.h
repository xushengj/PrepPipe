#ifndef PLAINTEXTOBJECT_H
#define PLAINTEXTOBJECT_H

#include "src/lib/ImportedObject.h"

#include <QObject>

class PlainTextObject : public ImportedObject
{
    Q_OBJECT
public:
    PlainTextObject()
        : ImportedObject(ObjectBase::ObjectType::Data_PlainText)
    {
        setImportConfigurationData(defaultConfig);
        setExportConfigurationData(defaultConfig);
    }
    explicit PlainTextObject(const QString& content)
        : PlainTextObject()
    {
        text = content;
    }
    PlainTextObject(const PlainTextObject&) = default;

    virtual PlainTextObject* clone() override {
        return new PlainTextObject(*this);
    }
    virtual bool save(QByteArray& dest) const override;

    virtual QWidget* getEditor() override;

    QString getText() const {return text;}
    void setText(const QString& t) {text = t;}

    virtual QString getFileNameFilter() const override;

public:
    static const ConfigurationDeclaration *getImportConfigurationDeclaration();
    static const ConfigurationDeclaration *getExportConfigurationDeclaration();
    static PlainTextObject* open(const QByteArray& src, const ConfigurationData& config);

private:
    static ConfigurationDeclaration importConfigDecl;
    static ConfigurationData defaultConfig;

private:
    QString text;
};

#endif // PLAINTEXTOBJECT_H
