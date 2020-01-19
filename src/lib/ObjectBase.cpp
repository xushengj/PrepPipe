#include "ObjectBase.h"

ObjectBase::ObjectBase(ObjectType Ty, const ConstructOptions &opt)
    : QObject(nullptr),
      name(opt.name), comment(opt.comment), nameSpace(opt.nameSpace),
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
    case ObjectType::GeneralTree:
        return tr("General Tree");
    }
    qFatal("Unhandled object type");
    return QString();
}

QIcon ObjectBase::getTypeDisplayIcon(ObjectType ty)
{
    switch(ty){
    case ObjectType::GeneralTree:
        return QIcon();
    }
    qFatal("Unhandled object type");
    return QIcon();
}
