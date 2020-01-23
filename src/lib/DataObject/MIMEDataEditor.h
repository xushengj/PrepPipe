#ifndef MIMEDATAEDITOR_H
#define MIMEDATAEDITOR_H

#include <QWidget>

namespace Ui {
class MIMEDataEditor;
}

class MIMEData;
class MIMEDataEditor : public QWidget
{
    Q_OBJECT

public:
    explicit MIMEDataEditor(MIMEData* srcPtr);
    ~MIMEDataEditor();

public slots:
    void refreshDataFromSrc();
    void copyToClipBoard();
    void currentFormatChanged(int formatIndex);

private:
    static QString getBinaryView(const QByteArray& data);

private:
    Ui::MIMEDataEditor *ui;
    MIMEData* const src;

    struct FormatData {
        QByteArray data;
        QStringList viewList;
        QStringList viewDataList;
        int currentView = -1;
    };

    QList<FormatData> formatDataList;

};

#endif // MIMEDATAEDITOR_H
