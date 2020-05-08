#include "SPMarkInputWidget.h"
#include "ui_SPMarkInputWidget.h"

SPMarkInputWidget::SPMarkInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SPMarkInputWidget)
{
    ui->setupUi(this);

    ui->typeComboBox->addItem(tr("Concatenation of following members"), QVariant(static_cast<int>(SimpleParser::BoundaryType::Concatenation)));
    ui->typeComboBox->addItem(tr("Any of following members or child marks"), QVariant(static_cast<int>(SimpleParser::BoundaryType::ClassBased)));

    connect(ui->typeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SPMarkInputWidget::markTypeChanged);


    elementCtl.init(ui->elementNameListWidget, ui->elementStackedWidget);
    elementCtl.setCreateWidgetCallback(std::bind(&SPMarkInputWidget::createElementInputWidget, this, std::placeholders::_1));
    connect(elementCtl.getObj(), &AnonymousElementListControllerObject::dirty, this, &SPMarkInputWidget::dirty);
}

SPMarkInputWidget::~SPMarkInputWidget()
{
    delete ui;
}

namespace {
int getIndexFromBoundaryType(SimpleParser::BoundaryType ty) {
    switch (ty) {
    case SimpleParser::BoundaryType::Concatenation:     return 0;
    case SimpleParser::BoundaryType::ClassBased:        return 1;
    default: qFatal("Unhandled named boundary type");   return 0;
    }
}
}

void SPMarkInputWidget::setNamedBoundaryCheckCallback(std::function<bool(const QString&)> cb)
{
    namedBoundaryCheckCB = cb;
    ui->parentListWidget->setCheckNameCallback(cb);
}

void SPMarkInputWidget::setData(const SimpleParser::NamedBoundary& dataArg)
{
    ui->typeComboBox->setCurrentIndex(getIndexFromBoundaryType(dataArg.ty));
    ui->parentListWidget->setData(dataArg.parentList);
    elementCtl.setData(dataArg.elements);
}

void SPMarkInputWidget::getData(const QString& name, SimpleParser::NamedBoundary& dataArg)
{
    dataArg.ty = static_cast<SimpleParser::BoundaryType>(ui->typeComboBox->currentData().toInt());
    dataArg.name = name;
    dataArg.parentList.clear();
    if (dataArg.ty == SimpleParser::BoundaryType::ClassBased) {
        dataArg.parentList = ui->parentListWidget->getData();
    }
    elementCtl.getData(dataArg.elements);
}

BoundaryDeclarationEditWidget* SPMarkInputWidget::createElementInputWidget(AnonymousElementListControllerObject* obj)
{
    BoundaryDeclarationEditWidget* w = new BoundaryDeclarationEditWidget;
    w->setNameListModel(namedBoundaryListModel);
    return w;
}

void SPMarkInputWidget::markTypeChanged()
{
    switch (ui->typeComboBox->currentData().toInt()) {
    default: break;
    case static_cast<int>(SimpleParser::BoundaryType::Concatenation): {
        ui->parentGroupBox->setEnabled(false);
    }break;
    case static_cast<int>(SimpleParser::BoundaryType::ClassBased): {
        ui->parentGroupBox->setEnabled(true);
    }break;
    }
}

void SPMarkInputWidget::nameUpdated(const QString& name)
{
    // no-op
    Q_UNUSED(name)
}
