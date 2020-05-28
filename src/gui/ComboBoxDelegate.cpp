#include "ComboBoxDelegate.h"

ComboBoxDelegate::ComboBoxDelegate(const QVector<ComboBoxRowRecord> &rows, QTableView* parentPtr)
    : QStyledItemDelegate(parentPtr),
      parent(parentPtr),
      data(rows)
{
    for (int i = 0, n = rows.size(); i < n; ++i) {
        const auto& row = rows.at(i);
        Q_ASSERT(row.data.isNull() == row.label.isEmpty());
    }
}

void ComboBoxDelegate::setComboBoxCurrentIndex(QComboBox* cbox, const QVariant& cellData) const
{
    for (int i = 0, n = data.size(); i < n; ++i) {
        if (data.at(i).data == cellData) {
            cbox->setCurrentIndex(i);
            return;
        }
    }
    qFatal("Unknown data for combobox");
}

QWidget* ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QComboBox* cbox = new QComboBox(parent);
    for (int i = 0, n = data.size(); i < n; ++i) {
        const auto& row = data.at(i);
        if (row.label.isEmpty()) {
            // separator
            cbox->insertSeparator(i);
        } else {
            // actual data
            cbox->addItem(row.icon, row.label, row.data);
        }
    }

    // reset focus when the combo box is changed
    // so that the table can be updated if change in this cell would cause change in other cells
    connect(cbox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){parent->setFocus();});

    return cbox;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox* cbox = qobject_cast<QComboBox*>(editor);
    Q_ASSERT(cbox);
    const QVariant& curData = index.data(Qt::EditRole);
    setComboBoxCurrentIndex(cbox, curData);
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox* cbox = qobject_cast<QComboBox*>(editor);
    Q_ASSERT(cbox);
    model->setData(index, cbox->currentData(), Qt::EditRole);
}
