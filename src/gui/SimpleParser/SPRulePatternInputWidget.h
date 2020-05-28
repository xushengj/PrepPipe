#ifndef SPRULEPATTERNINPUTWIDGET_H
#define SPRULEPATTERNINPUTWIDGET_H

#include <QWidget>
#include <QIcon>
#include <QListWidgetItem>

#include "src/lib/Tree/SimpleParser.h"

namespace Ui {
class SPRulePatternInputWidget;
}

class SPRulePatternInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SPRulePatternInputWidget(QWidget *parent = nullptr);
    ~SPRulePatternInputWidget();

    QString getPatternTypeName();

    void setData(const SimpleParser::Pattern& dataArg);
    void getData(SimpleParser::Pattern& dataArg);

    // the callback used to check whether a named boundary (marker) is valid
    void setNamedBoundaryCheckCallback(std::function<bool(const QString&)> cb) {
        namedBoundaryCheckCB = cb;
    }

    // the callback to check whether a content type is valid
    void setContentTypeCheckCallback(std::function<bool(const QString&)> cb) {
        contentTypeCheckCB = cb;
    }


public slots:
    // request to refresh check results
    void otherDataUpdated();

signals:
    void dirty();
    void patternTypeNameChanged();

private slots:
    void resetPatternDisplayFromData();
    void elementListContextMenuRequested(const QPoint& pos);

    void elementHoverStarted(QListWidgetItem* item);
    void elementHoverFinished(QListWidgetItem* item);

private:
    static QColor getContentTypeColor();
    static QColor getNamedBoundaryColor();
    static QColor getAnonymousBoundaryColor();
    static QColor getRegexBoundaryColor();
    static QColor getBoundaryColor(SimpleParser::PatternElement::ElementType ty);

private:
    Ui::SPRulePatternInputWidget *ui;

    SimpleParser::Pattern data;
    QIcon errorIcon;
    QVector<std::pair<int,int>> elementHighlightPosList; // for each element, what's the range in the patternDocument does it corresponds to

    std::function<bool(const QString&)> namedBoundaryCheckCB;
    std::function<bool(const QString&)> contentTypeCheckCB;
};

#endif // SPRULEPATTERNINPUTWIDGET_H
