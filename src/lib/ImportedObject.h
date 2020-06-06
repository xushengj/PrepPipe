#ifndef IMPORTEDOBJECT_H
#define IMPORTEDOBJECT_H

#include "src/lib/FileBackedObject.h"
#include "src/lib/Tree/Configuration.h"

#include <QObject>
#include <QIODevice>

#include <vector>

class ImportedObject;
class ImportFileDialog;
class FileImportSupportDecl {
    friend class ImportFileDialog;

private:
    static const std::vector<const FileImportSupportDecl*>& getInstanceVec();

public:
    FileImportSupportDecl();
    virtual ~FileImportSupportDecl() = default;

protected:
    static void * operator new(std::size_t);      // #1: To prevent allocation of scalar objects
    static void * operator new [] (std::size_t);  // #2: To prevent allocation of array of objects

protected:
    // functions that derived class should implement
    virtual QString getDisplayName() const = 0;
    virtual bool canOpen(const QByteArray& data, const ConfigurationData& config) const {Q_UNUSED(data) Q_UNUSED(config) return true;}
    virtual const ConfigurationDeclaration* getImportConfigurationDeclaration() const {return nullptr;}
    virtual ImportedObject* import(const QByteArray& data, const ConfigurationData& config) const = 0;
};

// note that while the import part is okay, the export part is not yet

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

    virtual void setExportConfigFromImportConfig() {}

    static ImportedObject* open(const QByteArray& src, QWidget* window);

    const ConfigurationData& getImportConfigurationData() const {return importConfig;}
    const ConfigurationData& getExportConfigurationData() const {return exportConfig;}

    void setImportConfigurationData(const ConfigurationData& config) {
        importConfig = config;
    }
    void setExportConfigurationData(const ConfigurationData& config) {
        exportConfig = config;
    }

    // dummy functions for ImportOptionDialog; derived types can provide better implementation
    static bool mayOpenAsThisObjectType(const QByteArray& src, ConfigurationData& config) {
        Q_UNUSED(src)
        Q_UNUSED(config)
        return true;
    }
    static const ConfigurationDeclaration* getImportConfigurationDeclaration() {return nullptr;}
    static const ConfigurationDeclaration* getExportConfigurationDeclaration() {return nullptr;}

private:
    ConfigurationData importConfig;
    ConfigurationData exportConfig;
};

#endif // IMPORTEDOBJECT_H
