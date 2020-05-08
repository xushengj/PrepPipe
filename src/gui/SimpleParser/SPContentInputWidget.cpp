#include "SPContentInputWidget.h"
#include "ui_SPContentInputWidget.h"

SPContentInputWidget::SPContentInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SPContentInputWidget)
{
    ui->setupUi(this);
}

SPContentInputWidget::~SPContentInputWidget()
{
    delete ui;
}

void SPContentInputWidget::setData(const SimpleParser::ContentType& dataArg)
{
    // TODO
    Q_UNUSED(dataArg)
}

void SPContentInputWidget::getData(const QString& name, SimpleParser::ContentType& dataArg)
{
    // TODO
    dataArg.name = name;
}

void SPContentInputWidget::nameUpdated(const QString& name)
{
    // no-op
    Q_UNUSED(name)
}
