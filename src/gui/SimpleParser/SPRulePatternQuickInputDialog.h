#ifndef SPRULEPATTERNQUICKINPUTDIALOG_H
#define SPRULEPATTERNQUICKINPUTDIALOG_H

#include <QDialog>
#include "src/gui/SimpleParser/SPRulePatternQuickInputSpecialBlockModel.h"

namespace Ui {
class SPRulePatternQuickInputDialog;
}

class SPRulePatternQuickInputDialog : public QDialog
{
    Q_OBJECT

public:
    struct HelperData {
        // for special block auto-completion
        SPRulePatternQuickInputSpecialBlockModel::SpecialBlockHelperData specialBlockHelper;
        QStringList whitespaceList = {QStringLiteral(" "), QStringLiteral("\t")}; // strings that are considered as white space
    };

public:
    explicit SPRulePatternQuickInputDialog(HelperData& helperRef, QString defaultNodeTypeName, QWidget *parent = nullptr);
    ~SPRulePatternQuickInputDialog();

private slots:
    void textEditContextMenuEventHandler(const QPoint& p);
    void text_markSelection();
    void text_contentUpdated();
    void scheduleRegeneratePattern();
    void regeneratePattern();

private:
    Ui::SPRulePatternQuickInputDialog *ui;
    HelperData& helper;
    bool isRegeneratePatternScheduled = false;

    SPRulePatternQuickInputSpecialBlockModel* specialBlockModel = nullptr;
};

#endif // SPRULEPATTERNQUICKINPUTDIALOG_H
