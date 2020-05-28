#include "SPRulePatternQuickInputDialog.h"
#include "ui_SPRulePatternQuickInputDialog.h"

#include "src/gui/ComboBoxDelegate.h"

#include <QMenu>

namespace {
QVector<ComboBoxDelegate::ComboBoxRowRecord> SpecialBlockTypeInfo;

void populateSpecialBlockTypeInfo_addRow(SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType data)
{
    ComboBoxDelegate::ComboBoxRowRecord record;
    record.label = SPRulePatternQuickInputSpecialBlockModel::getDisplayText(data);
    record.data = static_cast<int>(data);
    SpecialBlockTypeInfo.push_back(record);
}

void populateSpecialBlockTypeInfo_addSeparator()
{
    ComboBoxDelegate::ComboBoxRowRecord record;
    SpecialBlockTypeInfo.push_back(record);
}

void populateSpecialBlockTypeInfo()
{
    if (!SpecialBlockTypeInfo.isEmpty())
        return;

    SpecialBlockTypeInfo.reserve(5);
    populateSpecialBlockTypeInfo_addRow(SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType::ContentType);
    populateSpecialBlockTypeInfo_addRow(SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType::NamedBoundary);
    populateSpecialBlockTypeInfo_addSeparator();
    populateSpecialBlockTypeInfo_addRow(SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType::AnonymousBoundary_StringLiteral);
    populateSpecialBlockTypeInfo_addRow(SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType::AnonymousBoundary_Regex);
}
}

SPRulePatternQuickInputDialog::SPRulePatternQuickInputDialog(HelperData &helperRef, QString defaultNodeTypeName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SPRulePatternQuickInputDialog),
    helper(helperRef),
    specialBlockModel(nullptr)
{
    ui->setupUi(this);
    ui->textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit, &QWidget::customContextMenuRequested, this, &SPRulePatternQuickInputDialog::textEditContextMenuEventHandler);

    populateSpecialBlockTypeInfo();
    specialBlockModel = new SPRulePatternQuickInputSpecialBlockModel(helperRef.specialBlockHelper, ui->specialBlockTableView);
    ui->specialBlockTableView->setModel(specialBlockModel);
    ui->specialBlockTableView->horizontalHeader()->setStretchLastSection(true);
    ComboBoxDelegate* typeDelegate = new ComboBoxDelegate(SpecialBlockTypeInfo, ui->specialBlockTableView);
    ui->specialBlockTableView->setItemDelegateForColumn(SPRulePatternQuickInputSpecialBlockModel::COL_Type, typeDelegate);
    ui->specialBlockTableView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui->specialBlockTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->specialBlockTableView, &QWidget::customContextMenuRequested, specialBlockModel, &SPRulePatternQuickInputSpecialBlockModel::contextMenuHandler);

    SimpleParser::Pattern pattern;
    pattern.typeName = defaultNodeTypeName;
    ui->rulePreviewWidget->setData(pattern);

    connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &SPRulePatternQuickInputDialog::text_contentUpdated);
    connect(specialBlockModel, &SPRulePatternQuickInputSpecialBlockModel::tableUpdated, this, &SPRulePatternQuickInputDialog::scheduleRegeneratePattern);
}

SPRulePatternQuickInputDialog::~SPRulePatternQuickInputDialog()
{
    delete ui;
}

void SPRulePatternQuickInputDialog::textEditContextMenuEventHandler(const QPoint& p)
{
    QMenu menu(ui->textEdit);

    QTextCursor cursor = ui->textEdit->textCursor();
    if (cursor.hasSelection()) {
        // TODO add "clear marking" if there is any marked text block in the selection
        QAction* markAction = new QAction("Mark");
        connect(markAction, &QAction::triggered, this, &SPRulePatternQuickInputDialog::text_markSelection);
        menu.addAction(markAction);
    }

    if (!menu.isEmpty()) {
        menu.addSeparator();
    }

    // add lock/unlock correspondingly
    QAction* lockAction = new QAction(tr("Lock"));
    bool isLocked = ui->textEdit->isReadOnly();
    lockAction->setCheckable(true);
    lockAction->setChecked(isLocked);
    connect(lockAction, &QAction::triggered, this, [=](){
        ui->textEdit->setReadOnly(!isLocked);
    });
    menu.addAction(lockAction);

    // add "select all" if the text is not empty
    if (!ui->textEdit->document()->isEmpty()) {
        QAction* selectAllAction = new QAction(tr("Select All"));
        connect(selectAllAction, &QAction::triggered, ui->textEdit, &QPlainTextEdit::selectAll);
        menu.addAction(selectAllAction);
    }

    menu.exec(ui->textEdit->mapToGlobal(p));
}

void SPRulePatternQuickInputDialog::text_markSelection()
{
    QTextCursor cursor = ui->textEdit->textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    int textStart = cursor.selectionStart();
    int textEnd = cursor.selectionEnd();
    QString str = cursor.selectedText();
    int occurrenceIndex = 0;
    int lastPos = 0;
    while (true) {
        QTextCursor searchResult = cursor.document()->find(str, lastPos);

        // if we cannot find the text then our algorithm is broken...
        Q_ASSERT(!searchResult.isNull());

        int curTextStart = searchResult.anchor();
        int curTextEnd = searchResult.position();
        Q_ASSERT(curTextStart < curTextEnd);
        if (curTextStart == textStart) {
            Q_ASSERT(curTextEnd == textEnd);
            break;
        }
        occurrenceIndex += 1;
        lastPos = searchResult.selectionStart() + 1;
    }

    SPRulePatternQuickInputSpecialBlockModel::SpecialBlockRecord record;
    record.identifier.first = str;
    record.identifier.second = occurrenceIndex;
    record.textHighlightRange.first = textStart;
    record.textHighlightRange.second = textEnd;
    record.info.ty = SPRulePatternQuickInputSpecialBlockModel::SpecialBlockType::AnonymousBoundary_StringLiteral;
    record.info.str = str;
    specialBlockModel->addBlockRequested(record);
}

void SPRulePatternQuickInputDialog::text_contentUpdated()
{
    QString exampleText = ui->textEdit->toPlainText();
    specialBlockModel->exampleTextUpdated(exampleText);
    scheduleRegeneratePattern();
}

// ----------------------------------------------------------------------------
// pattern generation

void SPRulePatternQuickInputDialog::scheduleRegeneratePattern()
{
    if (isRegeneratePatternScheduled)
        return;

    isRegeneratePatternScheduled = true;
    QMetaObject::invokeMethod(this, &SPRulePatternQuickInputDialog::regeneratePattern, Qt::QueuedConnection);
}

namespace {

struct PatternElementData {
    SimpleParser::PatternElement data;
    bool isUserSpecified = false;
};

// these passes should only be called when the pattern list is not empty
// currently 4 passes are here:
// 1. for every string that contain EOL, break them up into substrings and EOLs (this must go before the 3rd pass so that it do not mess things up if whitespace list has '\n')
// 2. if the last element is not an EOL / a string ended with EOL, add an EOL
// 3. for every string, if one end of it contains whitespace, split out the whitespace and add an optional whitespace element (this must go before the next pass)
// 4. for every user specified element, at both end, if it is not adjacent to another user specified element or a whitespace, then insert an optional whitespace element
// 5. coalesce redundant elements (e.g. two consecutive optional whitespace, which will probably cause match failure...)
//      Note that this last pass is currently not implemented, because in no circumstance would the redundant element pattern appear
void pass_breakStringWithEOL(QList<PatternElementData>& pattern);
void pass_EOLTailPadding(QList<PatternElementData>& pattern);
void pass_breakStringEndingWithWhiteSpace(QList<PatternElementData>& pattern, const QStringList& whitespaceList);
void pass_insertWSAroundUserElement(QList<PatternElementData>& pattern, const QStringList &whitespaceList);
void pass_coalesceElements(QList<PatternElementData>& pattern);
}

void SPRulePatternQuickInputDialog::regeneratePattern()
{
    isRegeneratePatternScheduled = false;


    QList<PatternElementData> curPattern;
    QString exampleText = ui->textEdit->toPlainText();
    int textLength = exampleText.length();

    // step 1: build pattern

    auto addLiteral = [&](int start, int end) -> void {
        Q_ASSERT(start < end);
        QString text = exampleText.mid(start, end - start);
        PatternElementData elementData;
        elementData.isUserSpecified = false;
        elementData.data.ty = decltype (elementData.data.ty)::AnonymousBoundary_StringLiteral;
        elementData.data.str = text;
        curPattern.push_back(elementData);
    };

    const auto& specialBlocks = specialBlockModel->getData();
    int lastPos = 0;
    for (int i = 0, n = specialBlocks.size(); i < n; ++i) {
        const auto& curBlock = specialBlocks.at(i);
        int curBlockStart = curBlock.textHighlightRange.first;
        int curBlockEnd = curBlock.textHighlightRange.second;
        Q_ASSERT(curBlockEnd <= textLength);
        // ignore the ones that is not in the text
        // (they should have both block start and end to be -1)
        if (curBlockStart < lastPos)
            continue;

        Q_ASSERT(curBlockStart < curBlockEnd);

        if (lastPos != curBlockStart) {
            addLiteral(lastPos, curBlockStart);
        }

        // deal with this pattern element
        PatternElementData elementData;
        elementData.isUserSpecified = true;
        SimpleParser::PatternElement& element = elementData.data;
        element.elementName = curBlock.info.exportName;
        switch (curBlock.info.ty) {
        case decltype(curBlock.info.ty)::AnonymousBoundary_StringLiteral: {
            element.ty = decltype (element.ty)::AnonymousBoundary_StringLiteral;
            element.str = curBlock.identifier.first;
        }break;
        case decltype(curBlock.info.ty)::AnonymousBoundary_Regex: {
            element.ty = decltype (element.ty)::AnonymousBoundary_Regex;
            element.str = curBlock.info.str;
        }break;
        case decltype(curBlock.info.ty)::NamedBoundary: {
            element.ty = decltype (element.ty)::NamedBoundary;
            element.str = curBlock.info.str;
        }break;
        case decltype(curBlock.info.ty)::ContentType: {
            element.ty = decltype (element.ty)::Content;
            element.str = curBlock.info.str;
        }break;
        }
        curPattern.push_back(elementData);

        lastPos = curBlockEnd;
    }

    if (lastPos < textLength) {
        addLiteral(lastPos, textLength);
    }

    // step 2: common replacement
    if (!curPattern.isEmpty()) {
        pass_breakStringWithEOL(curPattern);
        pass_EOLTailPadding(curPattern);
        pass_breakStringEndingWithWhiteSpace(curPattern, helper.whitespaceList);
        pass_insertWSAroundUserElement(curPattern, helper.whitespaceList);
        pass_coalesceElements(curPattern);
    }

    // Done!
    SimpleParser::Pattern result;
    result.typeName = ui->rulePreviewWidget->getPatternTypeName();
    result.pattern.reserve(curPattern.size());
    for (int i = 0, n = curPattern.size(); i < n; ++i) {
        result.pattern.push_back(curPattern.at(i).data);
    }
    ui->rulePreviewWidget->setData(result);
}

namespace {
void pass_breakStringWithEOL(QList<PatternElementData>& pattern)
{
    Q_ASSERT(!pattern.isEmpty());

    for (int index = 0; index < pattern.size(); ++index) {
        // note that the pattern may grow during looping

        auto& curElement = pattern[index];

        // do not break non-strings or user specified strings
        if (curElement.isUserSpecified || curElement.data.ty != decltype (curElement.data.ty)::AnonymousBoundary_StringLiteral) {
            continue;
        }

        // invariant check
        Q_ASSERT(curElement.data.elementName.isEmpty());
        Q_ASSERT(!curElement.data.str.isEmpty());

        // ignore strings without EOL
        QString str = curElement.data.str; // implicitly shared
        if (!str.contains('\n')) {
            continue;
        }

        // make a copy of current element to avoid modifying the list while reading

        // okay, start breaking the element
        int lastPos = 0;
        while (true) {
            int nextPos = str.indexOf('\n', lastPos);
            if (nextPos == -1) {
                // this is the last string
                Q_ASSERT(lastPos > 0);
                PatternElementData tmp;
                tmp.data.ty = decltype (tmp.data.ty)::AnonymousBoundary_StringLiteral;
                tmp.data.str = str.mid(lastPos);
                pattern.insert(index, tmp);
                index += 1;
                break;
            } else {
                // we found a new string
                PatternElementData tmp;
                if (nextPos > lastPos) {
                    // this row is not empty
                    tmp.data.ty = decltype (tmp.data.ty)::AnonymousBoundary_StringLiteral;
                    tmp.data.str = str.mid(lastPos, nextPos - lastPos);
                    pattern.insert(index, tmp);
                    index += 1;
                }

                // now the '\n'
                tmp.data.ty = decltype (tmp.data.ty)::AnonymousBoundary_SpecialCharacter_LineFeed;
                tmp.data.str.clear();
                pattern.insert(index, tmp);
                index += 1;

                lastPos = nextPos + 1;
                if (lastPos >= str.length()) {
                    // no other characters left; we are done
                    break;
                }
            }
        }

        // remove the element we have removed
        pattern.removeAt(index);
        index -= 1;
    }
}

void pass_EOLTailPadding(QList<PatternElementData>& pattern)
{
    Q_ASSERT(!pattern.isEmpty());

    auto& last = pattern.back();
    if (last.data.ty != decltype (last.data.ty)::AnonymousBoundary_SpecialCharacter_LineFeed) {
        PatternElementData element;
        element.data.ty = decltype (last.data.ty)::AnonymousBoundary_SpecialCharacter_LineFeed;
        pattern.push_back(element);
    }
}

void pass_breakStringEndingWithWhiteSpace(QList<PatternElementData>& pattern, const QStringList &whitespaceList)
{
    Q_ASSERT(!pattern.isEmpty());

    for (int index = 0; index < pattern.size(); ++index) {
        // note that the pattern may grow during looping

        auto& curElement = pattern[index];

        // do not break non-strings or user specified strings
        if (curElement.isUserSpecified || curElement.data.ty != decltype (curElement.data.ty)::AnonymousBoundary_StringLiteral) {
            continue;
        }

        // invariant check
        Q_ASSERT(curElement.data.elementName.isEmpty());
        Q_ASSERT(!curElement.data.str.isEmpty());

        // check the beginning of string
        int wsLengthAtBeginning = 0;
        QStringRef strRef(&curElement.data.str);
        bool isChangeMade = true;
        while (isChangeMade) {
            isChangeMade = false;
            for (const auto& ws : whitespaceList) {
                if (strRef.startsWith(ws)) {
                    wsLengthAtBeginning += ws.length();
                    strRef = strRef.mid(ws.length());
                    isChangeMade = true;
                }
            }
        }

        // check the end of string
        // note that we don't do the replacement if the next element is a user specified string literal that starts with white space
        int wsLengthAtEnd = 0;
        bool isSkipWSTestAtEnd = false;
        if (index + 1 < pattern.size()) {
            const auto& nextElement = pattern.at(index + 1);
            if (nextElement.isUserSpecified && nextElement.data.ty == decltype (nextElement.data.ty)::AnonymousBoundary_StringLiteral) {
                for (const auto& ws : whitespaceList) {
                    if (nextElement.data.str.startsWith(ws)) {
                        isSkipWSTestAtEnd = true;
                        break;
                    }
                }
            }
        }
        if (!isSkipWSTestAtEnd && !strRef.isEmpty()) {
            isChangeMade = true;
            while (isChangeMade) {
                isChangeMade = false;
                for (const auto& ws : whitespaceList) {
                    if (strRef.endsWith(ws)) {
                        wsLengthAtEnd += ws.length();
                        strRef.chop(ws.length());
                        isChangeMade = true;
                    }
                }
            }
        }

        // first, if the string is just whitespace, we can modify it in place
        if (strRef.isEmpty()) {
            Q_ASSERT(wsLengthAtBeginning + wsLengthAtEnd == curElement.data.str.length());
            curElement.data.str.clear();
            curElement.data.ty = decltype (curElement.data.ty)::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace;
        } else {
            // we have some string left
            // if no whitespace to chop off, just continue
            if (wsLengthAtBeginning + wsLengthAtEnd == 0) {
                continue;
            }

            curElement.data.str = strRef.toString();
            // now avoid touching curElement
            if (wsLengthAtBeginning > 0) {
                PatternElementData tmp;
                tmp.data.ty = decltype (tmp.data.ty)::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace;
                pattern.insert(index++, tmp);
            }
            if (wsLengthAtEnd > 0) {
                PatternElementData tmp;
                tmp.data.ty = decltype (tmp.data.ty)::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace;
                pattern.insert(++index, tmp);
            }
        }
    }
}

void pass_insertWSAroundUserElement(QList<PatternElementData>& pattern, const QStringList& whitespaceList)
{
    Q_ASSERT(!pattern.isEmpty());

    for (int index = 0; index < pattern.size(); ++index) {
        // note that the pattern may grow during looping

        const auto& curElement = pattern.at(index);

        if (!curElement.isUserSpecified)
            continue;

        // if the previous element is not user specified AND it is not a whitespace, insert a whitespace at front
        // for front only: if the user specified element is a string and the string starts with a white space, do not add whitespace element in the front
        bool isInsertWSAtFront = false;
        bool isSkipFrontInsertTest = false;
        if (curElement.data.ty == decltype (curElement.data.ty)::AnonymousBoundary_StringLiteral) {
            for (const auto& ws : whitespaceList) {
                if (curElement.data.str.startsWith(ws)) {
                    isSkipFrontInsertTest = true;
                    break;
                }
            }
        }
        if (!isSkipFrontInsertTest) {
            if (index == 0) {
                isInsertWSAtFront = true;
            } else {
                const auto& prevElement = pattern.at(index - 1);
                if (!prevElement.isUserSpecified) {
                    switch (prevElement.data.ty) {
                    default: {
                        isInsertWSAtFront = true;
                    }break;
                    case decltype (prevElement.data.ty)::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace:
                    case decltype (prevElement.data.ty)::AnonymousBoundary_SpecialCharacter_WhiteSpaces: {
                        isInsertWSAtFront = false;
                    }
                    }
                }
            }
        }

        // similar logic for next element
        bool isInsertWSAtBack = false;
        if (index + 1 == pattern.size()) {
            // this is the last element
            // (at the time of writing, this is impossible, but just leave the code here)
            isInsertWSAtBack = true;
        } else {
            const auto& nextElement = pattern.at(index + 1);
            if (!nextElement.isUserSpecified) {
                switch (nextElement.data.ty) {
                default: {
                    isInsertWSAtBack = true;
                }break;
                case decltype (nextElement.data.ty)::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace:
                case decltype (nextElement.data.ty)::AnonymousBoundary_SpecialCharacter_WhiteSpaces: {
                    isInsertWSAtBack = false;
                }
                }
            }
        }

        if (isInsertWSAtFront || isInsertWSAtBack) {
            PatternElementData tmp;
            tmp.data.ty = decltype (tmp.data.ty)::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace;
            if (isInsertWSAtFront) {
                pattern.insert(index++, tmp);
            }
            if (isInsertWSAtBack) {
                pattern.insert(++index, tmp);
            }
        }
    }
}

void pass_coalesceElements(QList<PatternElementData>& pattern)
{
    Q_ASSERT(!pattern.isEmpty());
}

}
