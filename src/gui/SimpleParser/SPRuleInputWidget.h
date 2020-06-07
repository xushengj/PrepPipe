#ifndef SPRULEINPUTWIDGET_H
#define SPRULEINPUTWIDGET_H

#include <QWidget>
#include "src/lib/Tree/SimpleParser.h"
#include "src/gui/SimpleParser/SPRulePatternInputWidget.h"
#include "src/gui/SimpleParser/SPRulePatternQuickInputDialog.h"

namespace Ui {
class SPRuleInputWidget;
}

class SPRuleInputWidget : public QWidget
{
    Q_OBJECT

public:
    using StorageData = SimpleParser::MatchRuleNode;
    using HelperData = void;

    // for now the only shared common helper is for the quick input
    // we may create a struct if more than one thing shows up
    using CommonHelperData = SPRulePatternQuickInputDialog::HelperData;

public:
    explicit SPRuleInputWidget(QWidget *parent = nullptr);
    ~SPRuleInputWidget();

    void setData(const SimpleParser::MatchRuleNode& dataArg);
    void setData(const QString& name, const SimpleParser::MatchRuleNode& dataArg) {
        Q_UNUSED(name)
        setData(dataArg);
    }
    void bindCommonHelper(CommonHelperData& helper) {
        helperPtr = &helper;
    }
    void getData(const QString& name, SimpleParser::MatchRuleNode& dataArg);

    // Note: all the callback should be set before setting data

    // the callback to check whether a match rule node name is valid
    void setParentNodeNameCheckCallback(std::function<bool(const QString&)> cb);

    // the callback used to check whether a named boundary (marker) is valid
    void setNamedBoundaryCheckCallback(std::function<bool(const QString&)> cb) {
        namedBoundaryCheckCB = cb;
    }

    // the callback to check whether a content type is valid
    void setContentTypeCheckCallback(std::function<bool(const QString&)> cb) {
        contentTypeCheckCB = cb;
    }

public slots:
    void nameUpdated(const QString& name);

    // request to refresh check results
    void otherDataUpdated();

signals:
    void dirty();
    void gotoRuleNodeRequested(const QString& nodeName);

private:
    struct PatternData {
        SPRulePatternInputWidget* widget = nullptr;

    };

    static QString getPatternSummaryName(int index, const QString& outputTypeName);

private:
    void addPattern(const SimpleParser::Pattern& patternData);
    void updateButtonState();

private slots:
    void updatePatternName(SPRulePatternInputWidget* widget);
    void currentPatternChanged(int index);
    void addPatternRequested();
    void deleteCurrentPatternRequested();

private:
    Ui::SPRuleInputWidget *ui;

    std::function<bool(const QString&)> namedBoundaryCheckCB;
    std::function<bool(const QString&)> contentTypeCheckCB;

    QVector<PatternData> patterns;
    CommonHelperData* helperPtr = nullptr;
    QString defaultNodeTypeName;
};

#endif // SPRULEINPUTWIDGET_H
