#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "src/lib/Tree/Tree.h"

#include <QtGlobal>

class ConfigurationDeclaration
{
    friend class ConfigurationInputWidget;

public:
    enum class FieldType {
        String,
        Integer,
        FloatingPoint,
        Enum,
        Boolean,
        Group
    };
    struct Field {
        // basic info
        QString codeName;
        QString displayName;
        QString defaultValue;
        FieldType ty = FieldType::String;
        int indexOffsetFromParent = 0; // always a positive number

        // predicate (when this field is visible / enabled) (list of pred ORed)(list of pred ANDed)
        // if it is empty then it is always enabled
        // otherwise, this field is enabled iff all predicates in any one of the nested vector are satisfied (inner to outer: ANDed and ORed)
        // the predicates are evaluated at the group node; if the parent is not enabled then children are never enabled.
        QVector<QVector<Tree::Predicate>> predicates;

        // misc
        QStringList enumValue_DisplayValueList;
        QStringList enumValue_CodeValueList;

        QString boolValue_TrueCodeValue;
        QString boolValue_FalseCodeValue;
    };

public:
    int getNumFields() const {return fields.size();}
    const Field& getField(int index) const {return fields.at(index);}

public:
    // ConfigurationDeclaration is a constraint on tree; the data will be in ConfigurationData
    // At any moment which ConfigurationDeclaration do each ConfigurationData associate with should be clear
    ConfigurationDeclaration();
    explicit ConfigurationDeclaration(const QVector<Field>& fieldsArg);
    static ConfigurationDeclaration* get(const Tree& tree);

private:
    QVector<Field> fields;

};

class ConfigurationData : public Tree
{
public:
    ConfigurationData();
    explicit ConfigurationData(const Tree& src);
    bool isValid(const ConfigurationDeclaration& decl) const {Q_UNUSED(decl) return true;}

public:
    class Visitor {
    public:
        Visitor(const ConfigurationData& cfg)
            : config(cfg)
        {}
        Visitor(const ConfigurationData& cfg, int nodeIndex)
            : config(cfg), index(nodeIndex)
        {}
        Visitor operator[](const QString& codeName) const;
        QString operator()(const QString& codeName) const;
        operator bool() const {
            return isValid();
        }
        bool isValid() const {
            return index >= 0;
        }
        int getNodeIndex() const {
            return index;
        }
    private:
        const ConfigurationData& config;
        int index = 0;
    };

    Visitor operator[](const QString& codeName) const {
        return Visitor(*this)[codeName];
    }
    QString operator()(const QString& codeName) const {
        return Visitor(*this)(codeName);
    }
};

#endif // CONFIGURATION_H
