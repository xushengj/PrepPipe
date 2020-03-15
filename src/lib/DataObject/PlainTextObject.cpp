#include "PlainTextObject.h"
#include "src/gui/TextEditor.h"

#include <QTextStream>
#include <QTextCodec>
#include <QMessageBox>

ConfigurationDeclaration PlainTextObject::importConfigDecl;

namespace {
const QString CFG_CODEC = QStringLiteral("codec");
}

const ConfigurationDeclaration* PlainTextObject::getImportConfigurationDeclaration()
{
    if (importConfigDecl.getNumFields() == 0) {
        QVector<ConfigurationDeclaration::Field> fieldData;
        fieldData.reserve(1);
        ConfigurationDeclaration::Field codecField;
        codecField.ty = ConfigurationDeclaration::FieldType::Enum;
        codecField.codeName = CFG_CODEC;
        codecField.displayName = tr("Encoding");
        codecField.indexOffsetFromParent = 1;
        auto list = QTextCodec::availableCodecs();
        codecField.enumValue_CodeValueList.reserve(list.size());
        for (const QByteArray& ba : list) {
            codecField.enumValue_CodeValueList.push_back(QString::fromUtf8(ba));
        }
        codecField.enumValue_CodeValueList.sort(Qt::CaseInsensitive);
        codecField.enumValue_DisplayValueList = codecField.enumValue_CodeValueList;
        codecField.defaultValue = "UTF-8";
        Q_ASSERT(codecField.enumValue_CodeValueList.contains(codecField.defaultValue));
        fieldData.push_back(codecField);
        importConfigDecl = ConfigurationDeclaration(fieldData);
    }
    return &importConfigDecl;
}

PlainTextObject* PlainTextObject::open(const QByteArray &src, const ConfigurationData& config)
{
    QTextStream ts(src);
    QTextCodec* codec = QTextCodec::codecForName(config(CFG_CODEC).toUtf8());
    Q_ASSERT(codec);
    ts.setCodec(codec);
    QString text = ts.readAll();
    PlainTextObject* obj = new PlainTextObject(text);
    obj->setImportConfigurationData(config);
    return obj;
}

bool PlainTextObject::save(QByteArray& dest) const
{
    QTextStream ts(&dest, QIODevice::WriteOnly);
    QTextCodec* codec = QTextCodec::codecForName(getImportConfigurationData()(CFG_CODEC).toUtf8());
    Q_ASSERT(codec);
    ts.setCodec(codec);
    ts << text;
    return true;
}

QWidget* PlainTextObject::getEditor()
{
    TextEditor* edit = new TextEditor;
    edit->setPlainText(text);
    edit->document()->setModified(false);
    return edit;
}

bool PlainTextObject::editorNoUnsavedChanges(QWidget* editor)
{
    TextEditor* edit = qobject_cast<TextEditor*>(editor);
    Q_ASSERT(edit);
    return !edit->document()->isModified();
}

void PlainTextObject::saveDataFromEditor(QWidget* editor)
{
    TextEditor* edit = qobject_cast<TextEditor*>(editor);
    Q_ASSERT(edit);
    text = edit->toPlainText();
    edit->document()->setModified(false);
}

