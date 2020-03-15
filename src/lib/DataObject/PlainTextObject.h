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
    {}
    explicit PlainTextObject(const QString& content)
        : ImportedObject(ObjectBase::ObjectType::Data_PlainText),
          text(content)
    {}
    PlainTextObject(const PlainTextObject&) = default;

    virtual PlainTextObject* clone() override {
        return new PlainTextObject(*this);
    }
    virtual bool save(QByteArray& dest) const override;

    virtual QWidget* getEditor() override;
    virtual bool editorNoUnsavedChanges(QWidget* editor) override;
    virtual void saveDataFromEditor(QWidget* editor) override;

public:
    static const ConfigurationDeclaration *getImportConfigurationDeclaration();
    static PlainTextObject* open(const QByteArray& src, const ConfigurationData& config);

private:
    static ConfigurationDeclaration importConfigDecl;

private:
    QString text;
};

#endif // PLAINTEXTOBJECT_H
