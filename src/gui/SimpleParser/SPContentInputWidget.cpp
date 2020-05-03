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
