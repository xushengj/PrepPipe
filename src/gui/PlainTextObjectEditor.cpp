#include "PlainTextObjectEditor.h"
#include "src/lib/DataObject/PlainTextObject.h"
#include <QVBoxLayout>

PlainTextObjectEditor::PlainTextObjectEditor(QWidget *parent)
    : EditorBase(parent), editor(new TextEditor)
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(editor);
    setLayout(layout);
    connect(editor, &QPlainTextEdit::textChanged, this, &EditorBase::setDirty);
}

void PlainTextObjectEditor::saveToObjectRequested(ObjectBase *obj)
{
    PlainTextObject* textObj = qobject_cast<PlainTextObject*>(obj);
    Q_ASSERT(textObj);
    textObj->setText(editor->toPlainText());
    editor->document()->setModified(false);
    clearDirty();
}
