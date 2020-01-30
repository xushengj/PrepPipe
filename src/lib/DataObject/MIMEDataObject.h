#ifndef MIMEDATA_H
#define MIMEDATA_H

#include "src/lib/ObjectBase.h"
#include "src/gui/MIMEDataEditor.h"

#include <QObject>
#include <QMimeData>

class MIMEDataObject : public ObjectBase
{
    Q_OBJECT
public:
    MIMEDataObject(const ConstructOptions& opt);
    MIMEDataObject(const QMimeData& initData, const ConstructOptions& opt);
    virtual ~MIMEDataObject() override;

    const QHash<QString, QByteArray>& getData() const {return data;}

    static MIMEDataObject* dumpFromClipboard(); // null if no data

    virtual MIMEDataEditor* getEditor() override;
private:
    QHash<QString, QByteArray> data;
};

#endif // MIMEDATA_H
