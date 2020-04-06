#ifndef STGFRAGMENTINPUTWIDGET_H
#define STGFRAGMENTINPUTWIDGET_H

#include "src/utils/XMLUtilities.h"
#include "src/lib/Tree/SimpleTextGenerator.h"

#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextDocument>

namespace Ui {
class STGFragmentInputWidget;
}

class STGFragmentInputWidget : public QWidget
{
    Q_OBJECT

public:
    // the data for editor only
    struct EditorData {
        struct Mapping {
            QString exampleText;
            QString replacingParam;
        };
        QString fragmentText;
        QList<Mapping> mappings;
    };
public:
    explicit STGFragmentInputWidget(QWidget *parent = nullptr);
    ~STGFragmentInputWidget();

    void setTitle(const QString& label);
    void setData(const EditorData& editorData);
    void getData(EditorData& editorData) const;
    void setUnbacked();

    void updateReplacementListWidget();
    void refreshMappingListDerivedData();

public:
    static EditorData getDataFromStorage(const QVector<Tree::LocalValueExpression>& fragment, const QStringList& exampleText);

    static QVector<Tree::LocalValueExpression> getDataToStorage(QStringList& exampleText, const EditorData& src);

public slots:
    //void lock();
    //void unlock();
    //void clearDirty();

    void expansionTextChanged();

private slots:
    void paramNewRequested();
    void paramEditRequested(QListWidgetItem* item);
    void paramDeleteRequested(QListWidgetItem* item);
    void replacementListContextMenuRequested(const QPoint& pos);

signals:
    void dirty();

private:
    struct MappingGUIData {
        EditorData::Mapping baseData;
        QListWidgetItem* item = nullptr;
        int firstOccurrence = -1;
    };

    static QMultiMap<int, int> getStartToIndexMap(const QList<MappingGUIData>& replacementData); // get a mapping from first occurrence -> replacementData index

    bool execParamEditDialog(EditorData::Mapping& data);
    int getRecordIndex(QListWidgetItem* item) {
        int index = 0;
        for (const auto& repl : replacementData) {
            if (repl.item == item) {
                return index;
            }
            index += 1;
        }
        return -1;
    }
private:
    Ui::STGFragmentInputWidget *ui;

    QIcon validationFailIcon;

    // we order replacement mappings by the first occurrence of the text
    QList<MappingGUIData> replacementData;
};



#endif // STGFRAGMENTINPUTWIDGET_H
