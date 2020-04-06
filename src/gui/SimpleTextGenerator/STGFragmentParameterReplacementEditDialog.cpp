#include "STGFragmentParameterReplacementEditDialog.h"
#include "ui_STGFragmentParameterReplacementEditDialog.h"

#include <QTextBlock>

STGFragmentParameterReplacementEditDialog::STGFragmentParameterReplacementEditDialog(QString example, QString parameter, QTextDocument* docArg, QList<std::pair<int, int>> occupiedListArg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::STGFragmentParameterReplacementEditDialog),
    doc(docArg),
    occupiedList(occupiedListArg)
{
    ui->setupUi(this);

    ui->exampleLineEdit->setText(example);
    ui->exampleLineEdit->setToolTip(tr("What you want to replace in the example expanded text?"));
    ui->exampleLineEdit->setPlaceholderText(tr("(\"What to replace\" should not be empty)"));
    ui->parameterLineEdit->setText(parameter);
    ui->parameterLineEdit->setToolTip(tr("What value from visited node to use for replacing the place holder text? Leave it empty for node type"));
    ui->parameterLineEdit->setPlaceholderText(tr("<Node Type>"));

    if (example.isEmpty() && parameter.isEmpty()) {
        setWindowTitle(tr("New parameter replacement"));
    } else {
        setWindowTitle(tr("Edit parameter replacement: \"%1\" -> <%2>").arg(example, (parameter.isEmpty()? tr("<Node Type>") : parameter)));
    }

    connect(ui->exampleLineEdit, &QLineEdit::textChanged, this, &STGFragmentParameterReplacementEditDialog::updateLabelAndState);
    connect(ui->parameterLineEdit, &QLineEdit::textChanged, this, &STGFragmentParameterReplacementEditDialog::updateLabelAndState);

    // initialize the label immediately
    updateLabelAndState();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &STGFragmentParameterReplacementEditDialog::tryAccept);
}

STGFragmentParameterReplacementEditDialog::~STGFragmentParameterReplacementEditDialog()
{
    delete ui;
}

bool STGFragmentParameterReplacementEditDialog::validateState(bool updateLabel)
{
    QString exampleText = getResultExample();
    if (exampleText.isEmpty()) {
        if (updateLabel) {
            ui->exampleReplacementLabel->setText(tr("Given example should not be empty"));
        }
        return false;
    }

    QTextCursor cursor = doc->find(exampleText);
    if (cursor.isNull()) {
        if (updateLabel) {
            ui->exampleReplacementLabel->setText(tr("Example not found in template"));
        }
        return false;
    }

    int firstOccurrence = cursor.position();
    int nextCharPos = firstOccurrence + exampleText.length();
    for (const auto& interval : occupiedList) {
        if (!(interval.first >= nextCharPos || interval.second <= firstOccurrence)) {
            // overlap found
            if (updateLabel) {
                ui->exampleReplacementLabel->setText(tr("Example text overlap with other replacement example text"));
            }
            return false;
        }
    }

    if (updateLabel) {
        QString blk = cursor.block().text();
        int pos = cursor.positionInBlock();
        QString result = blk.mid(0, pos - exampleText.length());

        QString param = getResultParameter();
        if (param.isEmpty()) {
            result.append(tr("<Node Type>"));
        } else {
            result.append('<');
            result.append(param);
            result.append('>');
        }
        result.append(blk.mid(pos));

        QString label = QString("%1 -> %2").arg(blk.trimmed(), result.trimmed());
        ui->exampleReplacementLabel->setText(label);
    }

    return true;
}

void STGFragmentParameterReplacementEditDialog::updateLabelAndState()
{
    validateState(true);
}

void STGFragmentParameterReplacementEditDialog::tryAccept()
{
    if (validateState(true)) {
        accept();
    }
}

QString STGFragmentParameterReplacementEditDialog::getResultExample()
{
    return ui->exampleLineEdit->text();
}

QString STGFragmentParameterReplacementEditDialog::getResultParameter()
{
    return ui->parameterLineEdit->text();
}
