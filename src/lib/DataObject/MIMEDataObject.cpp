#include "MIMEDataObject.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QDateTime>

MIMEDataObject::MIMEDataObject(const ConstructOptions& opt)
    : ObjectBase(ObjectType::MIMEDataObject, opt)
{

}

MIMEDataObject::MIMEDataObject(const QMimeData& initData, const ConstructOptions& opt)
    : ObjectBase(ObjectType::MIMEDataObject, opt)
{
    QStringList formats = initData.formats();
    for (const auto& fmt : formats) {
        data.insert(fmt, initData.data(fmt));
    }
}

MIMEDataObject::~MIMEDataObject()
{

}

MIMEDataEditor* MIMEDataObject::getEditor()
{
    return new MIMEDataEditor(this);
}

MIMEDataObject* MIMEDataObject::dumpFromClipboard()
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
    return new MIMEDataObject(*data, opt);
}
