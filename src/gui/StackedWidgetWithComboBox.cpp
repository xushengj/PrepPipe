#include "StackedWidgetWithComboBox.h"
#include "ui_StackedWidgetWithComboBox.h"

StackedWidgetWithComboBox::StackedWidgetWithComboBox(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::StackedWidgetWithComboBox)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->placeholder);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StackedWidgetWithComboBox::comboBoxIndexChangeHandler);
}

StackedWidgetWithComboBox::~StackedWidgetWithComboBox()
{
    delete ui;
}

int	StackedWidgetWithComboBox::addWidget(QString name, QWidget* widget)
{
    Q_ASSERT(widgets.size() == widgetName.size());
    int index = widgets.size();
    widgets.push_back(widget);
    widgetName.push_back(name);
    ui->stackedWidget->addWidget(widget);
    ui->comboBox->addItem(name);
    if (widgets.size() == 1) {
        ui->comboBox->setCurrentIndex(index);
    }
    return index;
}

void StackedWidgetWithComboBox::setLabelText(QString text)
{
    ui->label->setText(text);
}

int	StackedWidgetWithComboBox::count() const
{
    return ui->comboBox->count();
}

int	StackedWidgetWithComboBox::currentIndex() const
{
    return ui->comboBox->currentIndex();
}

QWidget* StackedWidgetWithComboBox::currentWidget() const
{
    QWidget* w = ui->stackedWidget->currentWidget();
    if (w == ui->placeholder) {
        return nullptr;
    }
    return w;
}

int	StackedWidgetWithComboBox::indexOf(QWidget* widget) const
{
    return widgets.indexOf(widget);
}

QWidget* StackedWidgetWithComboBox::widget(int index) const
{
    return widgets.at(index);
}

void StackedWidgetWithComboBox::removeWidget(QWidget * widget)
{
    int index = indexOf(widget);
    if (index == -1) {
        return;
    }
    removeAt(index);
}

void StackedWidgetWithComboBox::removeAt(int index)
{
    if (index < 0 || index >= widgets.size())
        return;
    QWidget* widget = widgets.at(index);
    ui->stackedWidget->removeWidget(widget);
    ui->comboBox->removeItem(index);
    widgets.removeAt(index);
    widgetName.removeAt(index);
    if (widgets.size() <= index) {
        ui->comboBox->setCurrentIndex(index-1);
    } else {
        ui->comboBox->setCurrentIndex(index);
    }
}

void StackedWidgetWithComboBox::setCurrentIndex(int index)
{
    ui->comboBox->setCurrentIndex(index);
}

void StackedWidgetWithComboBox::setCurrentWidget(QWidget* widget) {
    setCurrentIndex(indexOf(widget));
}

void StackedWidgetWithComboBox::setCurrentWidget(const QString& name)
{
    setCurrentIndex(widgetName.indexOf(name));
}

void StackedWidgetWithComboBox::comboBoxIndexChangeHandler(int index)
{
    if (index >= 0 && index < widgets.size()) {
        ui->stackedWidget->setCurrentWidget(widgets.at(index));
    } else {
        index = -1;
        ui->stackedWidget->setCurrentWidget(ui->placeholder);
    }
    emit currentChanged(index);
}
