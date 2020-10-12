#ifndef STACKEDWIDGETWITHCOMBOBOX_H
#define STACKEDWIDGETWITHCOMBOBOX_H

#include <QWidget>
#include <QFrame>
#include <QList>
#include <QStringList>

namespace Ui {
class StackedWidgetWithComboBox;
}

class StackedWidgetWithComboBox : public QFrame
{
    Q_OBJECT

public:
    explicit StackedWidgetWithComboBox(QWidget *parent = nullptr);
    ~StackedWidgetWithComboBox();

    int	addWidget(QString name, QWidget* widget);
    int	count() const;
    int	currentIndex() const;
    QWidget* currentWidget() const;
    int	indexOf(QWidget* widget) const;
    int	insertWidget(int index, QString name, QWidget* widget);
    void removeWidget(QWidget * widget);
    void removeAt(int index);
    QWidget* widget(int index) const;

    void setLabelText(QString text);

public slots:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget* widget);
    void setCurrentWidget(const QString& name);

signals:
    void currentChanged(int index);

private slots:
    void comboBoxIndexChangeHandler(int index);

private:
    Ui::StackedWidgetWithComboBox *ui;
    QList<QWidget*> widgets;
    QStringList widgetName;
};

#endif // STACKEDWIDGETWITHCOMBOBOX_H
