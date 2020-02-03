#include "ObjectBase.h"

ObjectBase::ObjectBase(ObjectType Ty, const ConstructOptions &opt)
    : QObject(nullptr),
      name(opt.name), comment(opt.comment),
      filePath(opt.filePath), nameSpace(opt.nameSpace),
      ty(Ty), status(opt.isLocked? StatusFlag::Locked : StatusFlag::None)
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
    case ObjectType::GeneralTreeObject:
        return tr("General Tree");
    case ObjectType::MIMEDataObject:
        return tr("MIME");
    case ObjectType::SimpleTreeTransformObject:
        return tr("Tree-to-Tree Transform");
    case ObjectType::TestTaskObject:
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
