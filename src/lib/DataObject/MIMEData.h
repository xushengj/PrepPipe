#ifndef MIMEDATA_H
#define MIMEDATA_H

#include "src/lib/ObjectBase.h"
#include "src/lib/DataObject/MIMEDataEditor.h"

#include <QObject>
#include <QMimeData>

class MIMEData : public ObjectBase
{
    Q_OBJECT
public:
    MIMEData(const ConstructOptions& opt);
    MIMEData(const QMimeData& initData, const ConstructOptions& opt);
    virtual ~MIMEData() override;

    const QHash<QString, QByteArray>& getData() const {return data;}

    static MIMEData* dumpFromClipboard(); // null if no data

    virtual MIMEDataEditor* getEditor() override;
private:
    QHash<QString, QByteArray> data;
};

#endif // MIMEDATA_H
