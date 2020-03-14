#include "Configuration.h"

ConfigurationDeclaration::ConfigurationDeclaration()
{

}

ConfigurationDeclaration::ConfigurationDeclaration(const QVector<Field>& fieldsArg)
    : fields(fieldsArg)
{

}

ConfigurationDeclaration* ConfigurationDeclaration::get(const Tree& tree)
{
    // TODO
    Q_UNUSED(tree)
    return nullptr;
}

ConfigurationData::ConfigurationData(const Tree& src)
    : Tree(src)
{

}

