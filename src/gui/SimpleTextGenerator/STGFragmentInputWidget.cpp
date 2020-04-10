#include "STGFragmentInputWidget.h"
#include "ui_STGFragmentInputWidget.h"
#include "src/gui/SimpleTextGenerator/STGFragmentParameterReplacementEditDialog.h"

#include <QMenu>

STGFragmentInputWidget::STGFragmentInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::STGFragmentInputWidget),
    validationFailIcon(":/icon/execute/failed.png")
{
    ui->setupUi(this);
    setUnbacked();

    connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &STGFragmentInputWidget::expansionTextChanged);
    connect(ui->replacementListWidget, &QListWidget::itemDoubleClicked, this, &STGFragmentInputWidget::paramEditRequested);

    ui->replacementListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->replacementListWidget, &QListWidget::customContextMenuRequested, this, &STGFragmentInputWidget::replacementListContextMenuRequested);
}

STGFragmentInputWidget::~STGFragmentInputWidget()
{
    delete ui;
}

void STGFragmentInputWidget::setTitle(const QString& label)
{
    ui->groupBox->setTitle(label);
}

void STGFragmentInputWidget::setUnbacked()
{
    // clear data
    ui->textEdit->clear();
    ui->replacementListWidget->clear();
    replacementData.clear();

    ui->groupBox->setEnabled(false);
    ui->textEdit->setPlaceholderText(tr("Create an expansion rule first"));
}

void STGFragmentInputWidget::setData(const EditorData& editorData)
{
    // undo set unbacked
    ui->groupBox->setEnabled(true);
    ui->textEdit->setPlaceholderText(tr("Type expansion text here"));

    ui->textEdit->setPlainText(editorData.fragmentText);


    ui->replacementListWidget->clear();
    replacementData.clear();

    for (const auto& mapping : editorData.mappings) {
        MappingGUIData guiData;
        guiData.baseData = mapping;
        replacementData.push_back(guiData);
    }

    refreshMappingListDerivedData();
}

void STGFragmentInputWidget::getData(EditorData& editorData) const
{
    // replacementData should be up-to-date
    editorData.fragmentText = ui->textEdit->toPlainText();
    QMultiMap<int, int> startToIndexMap = getStartToIndexMap(replacementData);
    editorData.mappings.clear();
    for (auto iter = startToIndexMap.begin(), iterEnd = startToIndexMap.end(); iter != iterEnd; ++iter) {
        if (iter.key() < 0)
            continue;

        editorData.mappings.push_back(replacementData.at(iter.value()).baseData);
    }
}

void STGFragmentInputWidget::refreshMappingListDerivedData()
{
    QTextDocument* doc = ui->textEdit->document();

    // used to check overlapping between example texts
    // since we do not expect the number of replacements to be large, we just use a list
    QList<std::pair<int, int>> occupiedList;
    occupiedList.reserve(replacementData.size());

    for (auto& repl : replacementData) {
        repl.firstOccurrence = updateOccupiedList(occupiedList, repl.baseData, doc);
    }

    updateReplacementListWidget();
}

int STGFragmentInputWidget::updateOccupiedList(QList<std::pair<int,int>>& occupiedList, const EditorData::Mapping& mapping, QTextDocument* doc)
{
    int occurrenceTail = -1;
    if (!mapping.exampleText.isEmpty()) {
        // the cursor returned would point to end of string
        QTextCursor cursor = doc->find(mapping.exampleText);
        if (!cursor.isNull()) {
            occurrenceTail = cursor.position();
        } else {
            return -1;
        }
    }

    int occurrenceStart = occurrenceTail - mapping.exampleText.length();
    for (const auto& interval : occupiedList) {
        if (!(interval.first >= occurrenceTail || interval.second <= occurrenceStart)) {
            // overlap found
            return -1;
        }
    }

    // no overlap
    occupiedList.push_back(std::make_pair(occurrenceStart, occurrenceTail));
    return occurrenceStart;
}

QMultiMap<int, int> STGFragmentInputWidget::getStartToIndexMap(const QList<MappingGUIData>& replacementData)
{
    QMultiMap<int, int> startToIndexMap;

    int mappingIndex = 0;
    for (const auto& m : replacementData) {
        startToIndexMap.insert(m.firstOccurrence, mappingIndex);
        mappingIndex += 1;
    }

    return startToIndexMap;
}

void STGFragmentInputWidget::updateReplacementListWidget()
{
    // place them in order and update their appearence
    QMultiMap<int, int> startToIndexMap = getStartToIndexMap(replacementData);
    int listIndex = 0;
    for (auto iter = startToIndexMap.begin(), iterEnd = startToIndexMap.end(); iter != iterEnd; ++iter, ++listIndex) {
        int firstOccurrence = iter.key();
        int mappingIndex = iter.value();
        Q_ASSERT(mappingIndex >= 0 && mappingIndex < replacementData.size());
        auto& data = replacementData[mappingIndex];
        QListWidgetItem* item = data.item;
        if (item) {
            // we already have the item
            // update the state
            int currentRow = ui->replacementListWidget->row(item);
            if (currentRow != listIndex) {
                QListWidgetItem* itemDup = ui->replacementListWidget->takeItem(currentRow);
                Q_ASSERT(itemDup == item);
                ui->replacementListWidget->insertItem(listIndex, item);
            }
        } else {
            // create a new one
            item = new QListWidgetItem;
            QString keyName = data.baseData.replacingParam;
            if (keyName.isEmpty()) {
                keyName = tr("<Node Type>");
            }
            item->setText(QString("%1 -> %2").arg(keyName, data.baseData.exampleText));
            ui->replacementListWidget->insertItem(listIndex, item);
            data.item = item;
        }
        if (firstOccurrence >= 0) {
            item->setIcon(QIcon());
        } else {
            item->setIcon(validationFailIcon);
        }
    }
}

void STGFragmentInputWidget::expansionTextChanged()
{
    refreshMappingListDerivedData();
    emit dirty();
}

STGFragmentInputWidget::EditorData STGFragmentInputWidget::getDataFromStorage(const QVector<Tree::LocalValueExpression>& fragment, const QStringList& exampleText)
{
    // note that it is possible that the example text is missing
    STGFragmentInputWidget::EditorData data;
    int paramIndex = 0;
    Tree::Node node;
    for (const auto& expr : fragment) {
        if (expr.ty == decltype (expr.ty)::Literal)
            continue;
        QString value;
        if (paramIndex < exampleText.size()) {
            value = exampleText.at(paramIndex);
        }else {
            value.append('%');
            value.append(QString::number(paramIndex));
        }
        if (expr.ty == decltype (expr.ty)::KeyValue) {
            node.keyList.push_back(expr.str);
            node.valueList.push_back(value);

            EditorData::Mapping mapping;
            mapping.replacingParam = expr.str;
            mapping.exampleText = value;
            data.mappings.push_back(mapping);
        } else {
            node.typeName = value;

            EditorData::Mapping mapping;
            mapping.exampleText = value;
            data.mappings.push_back(mapping);
        }
        paramIndex += 1;
    }
    data.fragmentText.clear();
    bool isGood = SimpleTextGenerator::writeFragment(data.fragmentText, node, fragment, SimpleTextGenerator::EvaluationFailPolicy::Error);
    Q_ASSERT(isGood);
    return data;
}

QVector<Tree::LocalValueExpression> STGFragmentInputWidget::getDataToStorage(QStringList& exampleText, const EditorData& src)
{
    QVector<Tree::LocalValueExpression> result;
    exampleText.clear();

    // we need to discard invalid replacements
    QList<std::pair<int, int>> occupiedList;
    occupiedList.reserve(src.mappings.size());

    QMap<int, int> startToIndexMap;
    int mappingIndex = -1;
    for (const auto& mapping : src.mappings) {
        mappingIndex += 1;
        int firstOccurrence = src.fragmentText.indexOf(mapping.exampleText);
        if (firstOccurrence < 0)
            continue;
        int nextCharPos = firstOccurrence + mapping.exampleText.length();
        for (const auto& interval : occupiedList) {
            if (!(interval.first >= nextCharPos || interval.second <= firstOccurrence)) {
                // overlap found
                firstOccurrence = -1;
                break;
            }
        }
        if (firstOccurrence >= 0) {
            // no overlap
            occupiedList.push_back(std::make_pair(firstOccurrence, nextCharPos));
            startToIndexMap.insert(firstOccurrence, mappingIndex);
        }
    }

    int pos = 0;
    for (auto iter = startToIndexMap.begin(), iterEnd = startToIndexMap.end(); iter != iterEnd; ++iter) {
        int exampleStart = iter.key();
        int mappingIndex = iter.value();
        const auto& mapping = src.mappings.at(mappingIndex);
        int exampleEnd = exampleStart + mapping.exampleText.length();

        // emit a literal string first, if pos do not match
        if (pos < exampleStart) {
            Tree::LocalValueExpression expr;
            expr.ty = decltype (expr.ty)::Literal;
            expr.str = src.fragmentText.mid(pos, exampleStart - pos);
            result.push_back(expr);
        } else {
            Q_ASSERT(pos == exampleStart);
        }

        // now the parameters
        {
            Tree::LocalValueExpression expr;
            expr.ty = (mapping.replacingParam.isEmpty())? decltype (expr.ty)::NodeType : decltype (expr.ty)::KeyValue;
            expr.str = mapping.replacingParam;
            result.push_back(expr);
            exampleText.push_back(mapping.exampleText);
        }
        pos = exampleEnd;
    }

    if (pos < src.fragmentText.length()) {
        // add the last literal
        Tree::LocalValueExpression expr;
        expr.ty = decltype (expr.ty)::Literal;
        expr.str = src.fragmentText.mid(pos);
        result.push_back(expr);
    }

    return result;
}

void STGFragmentInputWidget::replacementListContextMenuRequested(const QPoint& pos)
{
    QListWidgetItem* item = ui->replacementListWidget->itemAt(pos);

    QMenu menu(ui->replacementListWidget);
    if (item) {
        QAction* editAction = new QAction(tr("Edit"));
        connect(editAction, &QAction::triggered, this, [=](){
            paramEditRequested(item);
        });
        menu.addAction(editAction);
        QAction* deleteAction = new QAction(tr("Delete"));
        connect(deleteAction, &QAction::triggered, this, [=](){
            paramDeleteRequested(item);
        });
        menu.addAction(deleteAction);
    } else {
        QAction* newAction = new QAction(tr("New"));
        connect(newAction, &QAction::triggered, this, &STGFragmentInputWidget::paramNewRequested);
        menu.addAction(newAction);
    }

    if (menu.isEmpty())
        return;

    menu.exec(ui->replacementListWidget->mapToGlobal(pos));
}

bool STGFragmentInputWidget::execParamEditDialog(EditorData::Mapping& data)
{
    QTextDocument* doc = ui->textEdit->document();

    QList<std::pair<int, int>> occupiedList;
    {
        // just build the occupied list
        occupiedList.reserve(replacementData.size());

        for (const auto& repl : replacementData) {
            const auto& mapping = repl.baseData;

            // skip the one we are editing
            if (mapping.exampleText == data.exampleText)
                continue;

            updateOccupiedList(occupiedList, mapping, doc);
        }
    }
    STGFragmentParameterReplacementEditDialog* dialog = new STGFragmentParameterReplacementEditDialog(data.exampleText, data.replacingParam, doc, occupiedList, this);
    int code = dialog->exec();
    dialog->deleteLater();
    if (code == QDialog::Accepted) {
        data.exampleText = dialog->getResultExample();
        data.replacingParam = dialog->getResultParameter();
        return true;
    }

    return false;
}

void STGFragmentInputWidget::paramEditRequested(QListWidgetItem* item)
{
    int row = getRecordIndex(item);
    Q_ASSERT(row >= 0);
    auto& data = replacementData[row];
    Q_ASSERT(data.item == item);
    if (execParamEditDialog(data.baseData)) {
        // force regeneration of label
        delete data.item;
        data.item = nullptr;
        refreshMappingListDerivedData();
        emit dirty();
    }
}

void STGFragmentInputWidget::paramNewRequested()
{
    EditorData::Mapping newMapping;
    if (execParamEditDialog(newMapping)) {
        MappingGUIData data;
        data.baseData = newMapping;
        replacementData.push_back(data);
        refreshMappingListDerivedData();
        emit dirty();
    }
}

void STGFragmentInputWidget::paramDeleteRequested(QListWidgetItem* item)
{
    int row = getRecordIndex(item);
    Q_ASSERT(replacementData.at(row).item == item);
    replacementData.removeAt(row);
    delete item;
    refreshMappingListDerivedData();
    emit dirty();
}
