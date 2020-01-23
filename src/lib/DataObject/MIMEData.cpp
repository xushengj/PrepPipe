#include "MIMEData.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QDateTime>

MIMEData::MIMEData(const ConstructOptions& opt)
    : ObjectBase(ObjectType::MIMEData, opt)
{

}

MIMEData::MIMEData(const QMimeData& initData, const ConstructOptions& opt)
    : ObjectBase(ObjectType::MIMEData, opt)
{
    QStringList formats = initData.formats();
    for (const auto& fmt : formats) {
        data.insert(fmt, initData.data(fmt));
    }
}

MIMEData::~MIMEData()
{
    if (editor) {
        editor->hide();
        editor->deleteLater();
    }
}

MIMEDataEditor* MIMEData::getEditor()
{
    if (!editor) {
        editor = new MIMEDataEditor(this);
    }
    return editor;
}

MIMEData* MIMEData::dumpFromClipboard()
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData(QClipboard::Clipboard);
    if (!data)
        return nullptr;

    QStringList formats = data->formats();
    if (formats.isEmpty())
        return nullptr;

    ObjectBase::ConstructOptions opt;
    opt.name = tr("Clipboard dump");
    opt.comment = tr("Time: %1").arg(QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate));
    return new MIMEData(*data, opt);
}
