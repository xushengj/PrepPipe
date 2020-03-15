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

ConfigurationData::ConfigurationData()
    : Tree()
{
    // insert the necessary root node
    nodes.push_back(Node());
}

ConfigurationData::ConfigurationData(const Tree& src)
    : Tree(src)
{

}

ConfigurationData::Visitor ConfigurationData::Visitor::operator[](const QString& codeName) const
{
    if (index == -1)
        return *this;

    // search for child node with given type
    for (int childOffset : config.getNode(index).offsetToChildren) {
        int childIndex = index + childOffset;
        const auto& node = config.getNode(childIndex);
        if (node.typeName == codeName) {
            return Visitor(config, childIndex);
        }
    }

    // not found
    return Visitor(config, -1);
}

QString ConfigurationData::Visitor::operator()(const QString& codeName) const
{
    if (index == -1)
        return QString();

    const auto& node = config.getNode(index);
    for (int i = 0, n = node.keyList.size(); i < n; ++i) {
        if (node.keyList.at(i) == codeName) {
            return node.valueList.at(i);
        }
    }

    // not found
    return QString();
}
