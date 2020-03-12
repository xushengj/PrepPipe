#include "MIMEDataObject.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QDateTime>

MIMEDataObject::MIMEDataObject()
    : ObjectBase(ObjectType::Data_MIME)
{

}

MIMEDataObject::MIMEDataObject(const QMimeData& initData)
    : ObjectBase(ObjectType::Data_MIME)
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

    MIMEDataObject* dumpObj = new MIMEDataObject(*data);
    dumpObj->setName(tr("Clipboard dump"));
    dumpObj->setComment(tr("Time: %1").arg(QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate)));
    return dumpObj;
}
