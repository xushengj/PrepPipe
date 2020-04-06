#ifndef STGFRAGMENTPARAMETERREPLACEMENTEDITDIALOG_H
#define STGFRAGMENTPARAMETERREPLACEMENTEDITDIALOG_H

#include <QDialog>
#include <QTextDocument>

namespace Ui {
class STGFragmentParameterReplacementEditDialog;
}

class STGFragmentParameterReplacementEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit STGFragmentParameterReplacementEditDialog(QString example, QString parameter, QTextDocument* docArg, QList<std::pair<int, int>> occupiedListArg, QWidget *parent = nullptr);
    ~STGFragmentParameterReplacementEditDialog();

    QString getResultExample();
    QString getResultParameter();

private slots:
    void updateLabelAndState();
    void tryAccept();

private:
    bool validateState(bool updateLabel);

private:
    Ui::STGFragmentParameterReplacementEditDialog *ui;
    QTextDocument* doc;

    QList<std::pair<int, int>> occupiedList;
};

#endif // STGFRAGMENTPARAMETERREPLACEMENTEDITDIALOG_H
