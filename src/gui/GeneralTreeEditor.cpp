#include "GeneralTreeEditor.h"
#include "ui_GeneralTreeEditor.h"

GeneralTreeEditor::GeneralTreeEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralTreeEditor)
{
    ui->setupUi(this);
}

GeneralTreeEditor::~GeneralTreeEditor()
{
    delete ui;
}
