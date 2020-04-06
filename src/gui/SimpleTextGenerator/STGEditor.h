#ifndef STGEDITOR_H
#define STGEDITOR_H

#include "src/gui/SimpleTextGenerator/SimpleTextGeneratorGUIObject.h"
#include "src/gui/SimpleTextGenerator/STGFragmentInputWidget.h"

#include <QWidget>

namespace Ui {
class STGEditor;
}

class STGEditor : public QWidget
{
    Q_OBJECT

public:
    struct RuleData {
        STGFragmentInputWidget::EditorData header;
        STGFragmentInputWidget::EditorData delimiter;
        STGFragmentInputWidget::EditorData tail;
        QStringList aliasList;
        // we want to remember data in gui but not writing back to storage if delimiter or tail is disabled
        bool isAllFieldsEnabled = false;
    };
    explicit STGEditor(QWidget *parent = nullptr);
    ~STGEditor();

    // functions that would be called from gui object
    void setBackingObject(SimpleTextGeneratorGUIObject* obj);
    bool isDirty() {
        return dirtyFlag;
    }
    void writeBack(SimpleTextGeneratorGUIObject* obj);

public slots:
    void tryGoToRule(int index);

private slots:
    void allFieldCheckboxToggled(bool checked);

private:
    RuleData& getData(int index) {
        auto iter = allData.find(canonicalNameList.at(index));
        Q_ASSERT(iter != allData.end());
        return iter.value();
    }

private:
    Ui::STGEditor *ui;

    SimpleTextGeneratorGUIObject* backedObj = nullptr;
    QHash<QString, RuleData> allData;
    bool dirtyFlag = false;
    bool errorOnUnknownNode = true;
    bool errorOnEvaluationFail = true;

    QStringList canonicalNameList;
    int currentIndex = -1; // current index in the sorted canonicalNameList; -1 if none is open
};

#endif // STGEDITOR_H
