#include "TaskObject.h"

TaskObject::TaskObject(ObjectType ty)
    : IntrinsicObject(ty)
{

}

class TaskObjectDecl : public IntrinsicObjectDecl
{
public:
    TaskObjectDecl() = default;
    virtual ~TaskObjectDecl() = default;

    virtual ObjectBase::ObjectType getObjectType() const override {
        return ObjectBase::ObjectType::TASK_START;
    }

    virtual ObjectBase::ObjectType getChildMax() const override {
        return ObjectBase::ObjectType::TASK_END;
    }

    virtual IntrinsicObject* create(const QString& name) const override {
        Q_UNUSED(name)
        qFatal("TaskObjectDecl::create() get called!");
        return nullptr;
    }
};
namespace {
TaskObjectDecl objDecl;
}

