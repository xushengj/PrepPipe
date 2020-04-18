#ifndef STGEDITOR_H
#define STGEDITOR_H

#include "src/gui/EditorBase.h"
#include "src/gui/SimpleTextGenerator/SimpleTextGeneratorGUIObject.h"
#include "src/gui/SimpleTextGenerator/STGFragmentInputWidget.h"

#include <QWidget>

namespace Ui {
class STGEditor;
}

class STGEditor : public EditorBase
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

    virtual void saveToObjectRequested(ObjectBase* obj) override;

public slots:
    void tryGoToRule(int index);

private slots:
    void allFieldCheckboxToggled(bool checked);

    void setDataDirty();

    void canonicalNameListContextMenuRequested(const QPoint& pos);
    void refreshCanonicalNameListWidget();

private:
    RuleData& getData(int index) {
        auto iter = allData.find(canonicalNameList.at(index));
        Q_ASSERT(iter != allData.end());
        return iter.value();
    }

    QString getCanonicalNameEditDialog(const QString& oldName, bool isNewInsteadofRename);

private:
    Ui::STGEditor *ui;

    SimpleTextGeneratorGUIObject* backedObj = nullptr;
    QHash<QString, RuleData> allData;
    bool errorOnUnknownNode = true;
    bool errorOnEvaluationFail = true;

    QStringList canonicalNameList;
    int currentIndex = -1; // current index in the sorted canonicalNameList; -1 if none is open
    bool isDuringRuleSwitch = false; // avoid bogus dirty signal during displayed rule switching
};

#endif // STGEDITOR_H
