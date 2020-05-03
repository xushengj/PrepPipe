#include "SPMarkInputWidget.h"
#include "ui_SPMarkInputWidget.h"

SPMarkInputWidget::SPMarkInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SPMarkInputWidget)
{
    ui->setupUi(this);
}

SPMarkInputWidget::~SPMarkInputWidget()
{
    delete ui;
}
