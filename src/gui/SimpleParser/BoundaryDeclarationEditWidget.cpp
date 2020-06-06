#include "BoundaryDeclarationEditWidget.h"
#include "ui_BoundaryDeclarationEditWidget.h"

#include "src/utils/NameSorting.h"

#include <QCompleter>

BoundaryDeclarationEditWidget::BoundaryDeclarationEditWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BoundaryDeclarationEditWidget)
{
    ui->setupUi(this);

    ui->originComboBox->addItem(tr("Use the value below"),          QVariant(static_cast<int>(SimpleParser::BoundaryDeclaration::DeclarationType::Value)));
    ui->originComboBox->addItem(tr("Reference by name"),            QVariant(static_cast<int>(SimpleParser::BoundaryDeclaration::DeclarationType::NameReference)));

    ui->valueTypeComboBox->addItem(tr("Text (String)"),             QVariant(static_cast<int>(SimpleParser::BoundaryType::StringLiteral)));
    ui->valueTypeComboBox->addItem(tr("Regular Expression"),        QVariant(static_cast<int>(SimpleParser::BoundaryType::Regex)));
    ui->valueTypeComboBox->addItem(tr("White Spaces (Optional)"),   QVariant(static_cast<int>(SimpleParser::BoundaryType::SpecialCharacter_OptionalWhiteSpace)));
    ui->valueTypeComboBox->addItem(tr("White Spaces (Mandatory)"),  QVariant(static_cast<int>(SimpleParser::BoundaryType::SpecialCharacter_WhiteSpaces)));
    ui->valueTypeComboBox->addItem(tr("New line"),                  QVariant(static_cast<int>(SimpleParser::BoundaryType::SpecialCharacter_LineFeed)));

    connect(ui->originComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(ui->valueTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BoundaryDeclarationEditWidget::valueTypeChanged);
    ui->originComboBox->setCurrentIndex(0);
    ui->valueTypeComboBox->setCurrentIndex(0);

    connect(ui->originComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BoundaryDeclarationEditWidget::dirty);
    // valueTypeChanged will emit dirty signal, so we don't need to connect it here
    connect(ui->valuePlainTextEdit, &QPlainTextEdit::textChanged, this, &BoundaryDeclarationEditWidget::dirty);
    connect(ui->referenceNameLineEdit, &QLineEdit::textChanged, this, &BoundaryDeclarationEditWidget::dirty);
}

void BoundaryDeclarationEditWidget::setNameListModel(QStringListModel* model)
{
    QCompleter* completer = new QCompleter(model);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setModelSorting(QCompleter::UnsortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    ui->referenceNameLineEdit->setCompleter(completer);
}

BoundaryDeclarationEditWidget::~BoundaryDeclarationEditWidget()
{
    delete ui;
}

namespace  {
int getValueTypeComboBoxIndex(SimpleParser::BoundaryType ty) {
    switch (ty) {
    case SimpleParser::BoundaryType::StringLiteral:                         return 0;
    case SimpleParser::BoundaryType::Regex:                                 return 1;
    case SimpleParser::BoundaryType::SpecialCharacter_OptionalWhiteSpace:   return 2;
    case SimpleParser::BoundaryType::SpecialCharacter_WhiteSpaces:          return 3;
    case SimpleParser::BoundaryType::SpecialCharacter_LineFeed:             return 4;
    default: qFatal("Unhandled boundary type for boundary declaration!");   return 0;
    }
}
}

void BoundaryDeclarationEditWidget::valueTypeChanged()
{
    int typeValue = ui->valueTypeComboBox->currentData().toInt();
    switch (typeValue) {
    case static_cast<int>(SimpleParser::BoundaryType::StringLiteral):
    case static_cast<int>(SimpleParser::BoundaryType::Regex): {
        ui->valuePlainTextEdit->setEnabled(true);
    }break;
    case static_cast<int>(SimpleParser::BoundaryType::SpecialCharacter_OptionalWhiteSpace):
    case static_cast<int>(SimpleParser::BoundaryType::SpecialCharacter_WhiteSpaces):
    case static_cast<int>(SimpleParser::BoundaryType::SpecialCharacter_LineFeed): {
        ui->valuePlainTextEdit->setEnabled(false);
    }break;
    }
    emit dirty();
}

void BoundaryDeclarationEditWidget::setData(const SimpleParser::BoundaryDeclaration& dataArg)
{
    switch (dataArg.decl) {
    case decltype(dataArg.decl)::Value: {
        ui->originComboBox->setCurrentIndex(0);
        ui->valueTypeComboBox->setCurrentIndex(getValueTypeComboBoxIndex(dataArg.ty));
        ui->valuePlainTextEdit->setPlainText(dataArg.str);
    }break;
    case decltype(dataArg.decl)::NameReference: {
        ui->originComboBox->setCurrentIndex(1);
        ui->referenceNameLineEdit->setText(dataArg.str);
    }break;
    }
}

void BoundaryDeclarationEditWidget::getData(SimpleParser::BoundaryDeclaration& dataArg)
{
    dataArg.decl = static_cast<decltype(dataArg.decl)>(ui->originComboBox->currentData().toInt());
    switch (dataArg.decl) {
    case decltype(dataArg.decl)::Value: {
        dataArg.ty = static_cast<decltype(dataArg.ty)>(ui->valueTypeComboBox->currentData().toInt());
        dataArg.str = ui->valuePlainTextEdit->toPlainText();
    }break;
    case decltype(dataArg.decl)::NameReference: {
        // this won't be used
        dataArg.ty = decltype (dataArg.ty)::StringLiteral;
        dataArg.str = ui->referenceNameLineEdit->text();
    }break;
    }
}

QString BoundaryDeclarationEditWidget::getDisplayLabel()
{
    SimpleParser::BoundaryDeclaration data;
    getData(data);
    switch (data.decl) {
    default: qFatal("Unhandled boundary declaration type");
    case decltype(data.decl)::Value: {
        switch (ui->valueTypeComboBox->currentData().toInt()) {
        case static_cast<int>(SimpleParser::BoundaryType::StringLiteral): {
            QString label;
            label.append('"');
            label.append(data.str);
            label.append('"');
            return label;
        }break;
        case static_cast<int>(SimpleParser::BoundaryType::Regex): {
            QString label;
            label.append(tr("RegEx: "));
            label.append(data.str);
            return label;
        }break;
        default: return ui->valueTypeComboBox->currentText();
        }
    }break;
    case decltype(data.decl)::NameReference: {
        QString label;
        label.append("<p style=\"color:blue;\"><i>");
        label.append(data.str);
        label.append("</i></p>");
        return label;
    }break;
    }
}
