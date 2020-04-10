#include "EditorBase.h"

EditorBase::EditorBase(QWidget *parent) : QWidget(parent)
{

}

void EditorBase::setDirty()
{
    dirtyFlag = true;
    emit dirty();
}
