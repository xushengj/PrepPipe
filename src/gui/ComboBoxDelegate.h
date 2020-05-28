#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>
#include <QTableView>
#include <QComboBox>
#include <QHash>

class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    struct ComboBoxRowRecord {
        QString label;  // what text to display for the row; empty string for a separator
        QVariant data;  // what data does the row represent
        QIcon icon;     // the icon to use
    };

public:
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

public:
    ComboBoxDelegate(const QVector<ComboBoxRowRecord>& rows, QTableView* parentPtr);

private:
    void setComboBoxCurrentIndex(QComboBox *cbox, const QVariant& cellData) const;

private:
    QTableView* parent;

    QVector<ComboBoxRowRecord> data;
};

#endif // COMBOBOXDELEGATE_H
