#ifndef MIMEDATAEDITOR_H
#define MIMEDATAEDITOR_H

#include <QWidget>

namespace Ui {
class MIMEDataEditor;
}

class MIMEDataObject;
class MIMEDataEditor : public QWidget
{
    Q_OBJECT

public:
    explicit MIMEDataEditor(MIMEDataObject* srcPtr);
    ~MIMEDataEditor();

public slots:
    void refreshDataFromSrc();
    void copyToClipBoard();
    void currentFormatChanged(int formatIndex);

private:
    static QString getBinaryView(const QByteArray& data);

private:
    Ui::MIMEDataEditor *ui;
    MIMEDataObject* const src;

    struct FormatData {
        QByteArray data;
        QStringList viewList;
        QStringList viewDataList;
        int currentView = -1;
    };

    QList<FormatData> formatDataList;

};

#endif // MIMEDATAEDITOR_H
