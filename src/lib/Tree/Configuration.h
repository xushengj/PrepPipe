#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QtGlobal>

class ConfigurationDeclaration
{
public:
    // ConfigurationDeclaration is a constraint on tree; the data will be in ConfigurationData
    // At any moment which ConfigurationDeclaration do each ConfigurationData associate with should be clear
    ConfigurationDeclaration();
};

class ConfigurationData
{
public:
    bool isValid(const ConfigurationDeclaration& decl) const {Q_UNUSED(decl) return true;}
};

#endif // CONFIGURATION_H
