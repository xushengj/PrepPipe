#ifndef SPRULEPATTERNQUICKINPUTDIALOG_H
#define SPRULEPATTERNQUICKINPUTDIALOG_H

#include <QDialog>
#include "src/lib/Tree/SimpleParser.h"
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

    void getData(SimpleParser::Pattern& data);

private slots:
    void textEditContextMenuEventHandler(const QPoint& p);
    void text_markSelection();
    bool hasInSelectionUnmarkedOccurrenceOfSpecialRegex(const QString& fullText, SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType ty);
    bool hasInSelectionUnmarkedOccurrenceOf(const QString& fullText, const QString& existingMarkedText);
    void text_markInSelectionAllOccurrenceOfSpecialRegex(SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType ty);
    void text_markInSelectionAllOccurrenceOf(const QString& existingMarkedText, const SPRulePatternQuickInputSpecialBlockModel::SpecialBlockInfo& info);
    void text_contentUpdated();
    void scheduleRegeneratePattern();
    void regeneratePattern();

private:
    int findOccurrenceIndex(const QString& str, int textStart, int textEnd);
private:
    Ui::SPRulePatternQuickInputDialog *ui;
    HelperData& helper;
    bool isRegeneratePatternScheduled = false;

    SPRulePatternQuickInputSpecialBlockModel* specialBlockModel = nullptr;
};

#endif // SPRULEPATTERNQUICKINPUTDIALOG_H
