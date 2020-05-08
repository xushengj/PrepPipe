#ifndef BOUNDARYDECLARATIONEDITWIDGET_H
#define BOUNDARYDECLARATIONEDITWIDGET_H

#include <QWidget>
#include <QStringListModel>

#include "src/lib/Tree/SimpleParser.h"

namespace Ui {
class BoundaryDeclarationEditWidget;
}

class BoundaryDeclarationEditWidget : public QWidget
{
    Q_OBJECT

public:
    using StorageData = SimpleParser::BoundaryDeclaration;
    using HelperData = void;

public:
    explicit BoundaryDeclarationEditWidget(QWidget *parent = nullptr);
    ~BoundaryDeclarationEditWidget();

    void setNameListModel(QStringListModel* model);

    void setData(const SimpleParser::BoundaryDeclaration& dataArg);
    void getData(SimpleParser::BoundaryDeclaration& dataArg);

    QString getDisplayLabel();

signals:
    void dirty();

private slots:
    void valueTypeChanged();

private:
    Ui::BoundaryDeclarationEditWidget *ui;
};

#endif // BOUNDARYDECLARATIONEDITWIDGET_H
