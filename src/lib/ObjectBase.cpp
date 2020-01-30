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
    }
    qFatal("Unhandled object type");
    return QString();
}

QIcon ObjectBase::getTypeDisplayIcon(ObjectType ty)
{
    switch(ty){
    case ObjectType::GeneralTreeObject:
        return QIcon(":/icon/test.png");
    case ObjectType::MIMEDataObject:
        return QIcon(":/icon/test.png");
    case ObjectType::SimpleTreeTransformObject:
        return QIcon(":/icon/test.png");
    }
    qFatal("Unhandled object type");
    return QIcon();
}
