#include "ObjectBase.h"

ObjectBase::ObjectBase(ObjectType Ty)
    : QObject(nullptr), ty(Ty), status(StatusFlag::None)
{

}

ObjectBase::ObjectBase(ObjectType Ty, const QString& nameArg)
    : QObject(nullptr), name(nameArg), ty(Ty), status(StatusFlag::None)
{

}

ObjectBase::ObjectBase(const ObjectBase& src)
    : QObject(nullptr),
      name(src.name),
      comment(src.comment),
      nameSpace(src.nameSpace),
      ty(src.ty),
      status(src.status)
{

}

QString ObjectBase::getTypeClassName(ObjectType ty)
{
    return QString(QMetaEnum::fromType<ObjectType>().valueToKey(ty));
}

QString ObjectBase::prettyPrintNameSpace(const QStringList& nameSpace)
{
    QString result;
    result.append('[');
    result.append(nameSpace.join(" -> "));
    result.append(']');
    return result;
}

ObjectBase::NamedReference::NamedReference(QString expr)
{
    QStringList list = expr.split('/');
    if (!list.isEmpty()) {
        if (list.size() == 1) {
            name = list.front();
        } else {
            name = list.back();
            list.pop_back();
            nameSpace = list;
        }
    }
}

QString ObjectBase::NamedReference::prettyPrint() const
{
    if (nameSpace.isEmpty())
        return name;

    QString result(name);
    result.append(" @ ");
    result.append(prettyPrintNameSpace(nameSpace));
    return result;
}

QString ObjectBase::NamedReference::prettyPrint(const QStringList& mainNameSpace) const
{
    const QStringList& nameSpaceToUse = nameSpace.isEmpty()? mainNameSpace: nameSpace;
    if (nameSpaceToUse.isEmpty())
        return name;

    QString result(name);
    result.append(" @ ");
    result.append(prettyPrintNameSpace(nameSpaceToUse));
    return result;
}

//*****************************************************************************
// Object type "registration"

QString ObjectBase::getTypeDisplayName(ObjectType ty)
{
    switch(ty){
    case ObjectType::Data_GeneralTree:
        return tr("General Tree");
    case ObjectType::Data_PlainText:
        return tr("Plain Text");
    case ObjectType::Data_MIME:
        return tr("MIME");
    case ObjectType::Task_SimpleTreeTransform:
        return tr("Tree-to-Tree Transform");
    case ObjectType::Task_SimpleParser:
        return tr("Plain Text Parser");
    case ObjectType::Task_SimpleTextGenerator:
        return tr("Plain Text Generator");
    case ObjectType::Task_SimpleWorkflow:
        return tr("Workflow");
    case ObjectType::Task_Test:
        return tr("Test");
    default: qFatal("Unhandled object type"); return QString();
    }
}

QIcon ObjectBase::getTypeDisplayIcon(ObjectType ty)
{
    switch(ty){
    default: return QIcon(":/icon/test.png");
    }
    qFatal("Unhandled object type");
    return QIcon();
}
