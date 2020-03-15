#ifndef IMPORTEDOBJECT_H
#define IMPORTEDOBJECT_H

#include "src/lib/FileBackedObject.h"
#include "src/lib/Tree/Configuration.h"

#include <QObject>
#include <QIODevice>

class ImportedObject : public FileBackedObject
{
    Q_OBJECT
public:
    explicit ImportedObject(ObjectType ty)
        : FileBackedObject(ty)
    {}
    ImportedObject(ObjectType ty, const ConfigurationData& import)
        : FileBackedObject(ty), importConfig(import)
    {}
    ImportedObject(const ImportedObject&) = default;

    virtual ~ImportedObject() override {}

    virtual bool saveToFile() override final;

    virtual bool save(QByteArray& dest) const = 0;

    static ImportedObject* open(const QByteArray& src, QWidget* window);

    const ConfigurationData& getImportConfigurationData() const {return importConfig;}
    void setImportConfigurationData(const ConfigurationData& config) {
        importConfig = config;
    }

    // dummy functions for ImportOptionDialog; derived types can provide better implementation
    static bool mayOpenAsThisObjectType(const QByteArray& src, ConfigurationData& config) {
        Q_UNUSED(src)
        Q_UNUSED(config)
        return true;
    }
    static const ConfigurationDeclaration* getImportConfigurationDeclaration() {return nullptr;}


private:
    ConfigurationData importConfig;
};

#endif // IMPORTEDOBJECT_H
