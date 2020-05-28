#include "SPRulePatternInputWidget.h"
#include "ui_SPRulePatternInputWidget.h"

#include "src/misc/Settings.h"

#include <QPlainTextDocumentLayout>
#include <QRegularExpression>

SPRulePatternInputWidget::SPRulePatternInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SPRulePatternInputWidget),
    errorIcon(":/icon/execute/failed.png")
{
    ui->setupUi(this);

    ui->plainTextEdit->setReadOnly(true);

    ui->elementListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->elementListWidget, &QWidget::customContextMenuRequested, this, &SPRulePatternInputWidget::elementListContextMenuRequested);
    connect(ui->elementListWidget, &ElementListWidget::itemHoverStarted, this, &SPRulePatternInputWidget::elementHoverStarted);
    connect(ui->elementListWidget, &ElementListWidget::itemHoverFinished, this, &SPRulePatternInputWidget::elementHoverFinished);

    connect(ui->typeNameLineEdit, &QLineEdit::editingFinished, this, &SPRulePatternInputWidget::patternTypeNameChanged);
}

SPRulePatternInputWidget::~SPRulePatternInputWidget()
{
    delete ui;
}

QString SPRulePatternInputWidget::getPatternTypeName()
{
    data.typeName = ui->typeNameLineEdit->text();
    return data.typeName;
}

void SPRulePatternInputWidget::getData(SimpleParser::Pattern& dataArg)
{
    data.typeName = ui->typeNameLineEdit->text();
    dataArg = data;
}

void SPRulePatternInputWidget::setData(const SimpleParser::Pattern& dataArg)
{
    data = dataArg;
    ui->typeNameLineEdit->setText(data.typeName);
    resetPatternDisplayFromData();
}

namespace {
const QString SETTING_COLOR_CONTENTTYPE = QStringLiteral("SimpleParserGUI/Color_ContentType");
const QString SETTING_COLOR_NAMED_BOUNDARY = QStringLiteral("SimpleParserGUI/Color_NamedBoundary");
const QString SETTING_COLOR_ANONYMOUS_BOUNDARY = QStringLiteral("SimpleParserGUI/Color_AnonymousBoundary");
const QString SETTING_COLOR_REGEX_BOUNDARY = QStringLiteral("SimpleParserGUI/Color_RegexBoundary");
}

QColor SPRulePatternInputWidget::getContentTypeColor()
{
    // default to blue
    return Settings::inst()->value(SETTING_COLOR_CONTENTTYPE, QColor(0, 64, 168)).value<QColor>();
}

QColor SPRulePatternInputWidget::getNamedBoundaryColor()
{
    // default to green
    return Settings::inst()->value(SETTING_COLOR_NAMED_BOUNDARY, QColor(0, 128, 32)).value<QColor>();
}

QColor SPRulePatternInputWidget::getAnonymousBoundaryColor()
{
    // default to black
    return Settings::inst()->value(SETTING_COLOR_ANONYMOUS_BOUNDARY, QColor(0, 0, 0)).value<QColor>();
}

QColor SPRulePatternInputWidget::getRegexBoundaryColor()
{
    // default to red
    return Settings::inst()->value(SETTING_COLOR_REGEX_BOUNDARY, QColor(128, 0, 0)).value<QColor>();
}

QColor SPRulePatternInputWidget::getBoundaryColor(SimpleParser::PatternElement::ElementType ty)
{
    switch (ty) {
    case SimpleParser::PatternElement::ElementType::Content: return getContentTypeColor();
    case SimpleParser::PatternElement::ElementType::NamedBoundary: return getNamedBoundaryColor();
    case SimpleParser::PatternElement::ElementType::AnonymousBoundary_Regex: return getRegexBoundaryColor();
    default: return getAnonymousBoundaryColor();
    }
}

void SPRulePatternInputWidget::otherDataUpdated()
{
    resetPatternDisplayFromData();
}

void SPRulePatternInputWidget::resetPatternDisplayFromData()
{
    ui->elementListWidget->clear();
    elementHighlightPosList.clear();
    QColor contentColor = getContentTypeColor();
    QColor anonymousBoundaryColor = getAnonymousBoundaryColor();
    QColor namedBoundaryColor = getNamedBoundaryColor();
    QColor regexBoundaryColor = getRegexBoundaryColor();
    ui->plainTextEdit->clear();
    QTextCursor cursor(ui->plainTextEdit->document());
    QTextCharFormat fmt;
    QChar wsDisplayChar(0x2423);
    for (auto& element : data.pattern) {
        int index = elementHighlightPosList.size();
        QListWidgetItem* item = new QListWidgetItem;
        item->setData(Qt::UserRole, index);

        // these are the data that should be populated in the big switch
        QString itemText = QString("[%1] ").arg(QString::number(index));
        if (!element.elementName.isEmpty()) {
            itemText.append(QString("[%1] ").arg(element.elementName));
        }
        QString patternText;
        QColor currentColor;
        bool isInError = false;
        switch (element.ty) {
        case decltype(element.ty)::Content: {
            currentColor = contentColor;
            itemText.append(element.str);
            patternText.append('<');
            patternText.append(element.str);
            patternText.append('>');
            if (contentTypeCheckCB) {
                if (!contentTypeCheckCB(element.str)) {
                    isInError = true;
                }
            }
        }break;
        case decltype(element.ty)::NamedBoundary: {
            currentColor = namedBoundaryColor;
            itemText.append(element.str);
            patternText.append('<');
            patternText.append(element.str);
            patternText.append('>');
            if (namedBoundaryCheckCB) {
                if (!namedBoundaryCheckCB(element.str)) {
                    isInError = true;
                }
            }
        }break;
        case decltype(element.ty)::AnonymousBoundary_StringLiteral: {
            currentColor = anonymousBoundaryColor;
            itemText.append('"');
            QString str = element.str;
            str.replace(' ', wsDisplayChar);
            itemText.append(str);
            itemText.append('"');
            patternText.append(element.str);
        }break;
        case decltype(element.ty)::AnonymousBoundary_Regex: {
            currentColor = regexBoundaryColor;
            QString regexDisplay = tr("regex: %1").arg(element.str);
            itemText.append(regexDisplay);
            patternText.append('<');
            patternText.append(regexDisplay);
            patternText.append('>');
            {
                // make sure this matches with the one used in SimpleParser
                QRegularExpression regex(element.str, QRegularExpression::MultilineOption | QRegularExpression::DontCaptureOption | QRegularExpression::UseUnicodePropertiesOption);
                isInError = !regex.isValid();
            }
        }break;
        case decltype(element.ty)::AnonymousBoundary_SpecialCharacter_LineFeed: {
            currentColor = anonymousBoundaryColor;
            QChar lfDisplayChar(0x23CE);
            itemText.append(lfDisplayChar);
            patternText.append(lfDisplayChar);
            patternText.append('\n');
        }break;
        case decltype(element.ty)::AnonymousBoundary_SpecialCharacter_WhiteSpaces: {
            currentColor = anonymousBoundaryColor;
            itemText.append(tr("Whitespace (Mandatory)"));
            patternText.append(wsDisplayChar);
        }break;
        case decltype(element.ty)::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace: {
            currentColor = anonymousBoundaryColor;
            itemText.append(tr("Whitespace (Optional)"));
            patternText.append(' ');
        }break;
        }
        int startPos = cursor.position();
        fmt.setForeground(QBrush(currentColor));
        cursor.insertText(patternText, fmt);
        int endPos = cursor.position();
        Q_ASSERT(startPos < endPos);
        elementHighlightPosList.push_back(std::make_pair(startPos, endPos));
        item->setForeground(currentColor);
        item->setText(itemText);
        if (isInError) {
            item->setIcon(errorIcon);
        }
        ui->elementListWidget->addItem(item);
    }
    ui->elementListWidget->resetHoverData();
}

void SPRulePatternInputWidget::elementHoverStarted(QListWidgetItem* item)
{
    bool isOk = false;
    int index = item->data(Qt::UserRole).toInt(&isOk);
    Q_ASSERT(isOk && index >= 0 && index < elementHighlightPosList.size());

    QTextCharFormat fmt;
    fmt.setForeground(getBoundaryColor(data.pattern.at(index).ty));
    fmt.setFontWeight(QFont::Bold);
    fmt.setBackground(Qt::yellow);
    QTextCursor cursor(ui->plainTextEdit->document());
    const auto& p = elementHighlightPosList.at(index);
    cursor.setPosition(p.first, QTextCursor::MoveAnchor);
    cursor.setPosition(p.second, QTextCursor::KeepAnchor);
    cursor.setCharFormat(fmt);
}

void SPRulePatternInputWidget::elementHoverFinished(QListWidgetItem* item)
{
    bool isOk = false;
    int index = item->data(Qt::UserRole).toInt(&isOk);
    Q_ASSERT(isOk && index >= 0 && index < elementHighlightPosList.size());

    QTextCharFormat fmt;
    fmt.setForeground(getBoundaryColor(data.pattern.at(index).ty));
    QTextCursor cursor(ui->plainTextEdit->document());
    const auto& p = elementHighlightPosList.at(index);
    cursor.setPosition(p.first, QTextCursor::MoveAnchor);
    cursor.setPosition(p.second, QTextCursor::KeepAnchor);
    cursor.setCharFormat(fmt);
}

void SPRulePatternInputWidget::elementListContextMenuRequested(const QPoint& pos)
{
    // TODO
}

