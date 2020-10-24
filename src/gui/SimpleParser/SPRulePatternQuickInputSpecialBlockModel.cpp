#include "SPRulePatternQuickInputSpecialBlockModel.h"

#include <QMenu>
#include <QAction>

#include <vector>
#include <algorithm>

SPRulePatternQuickInputSpecialBlockModel::SPRulePatternQuickInputSpecialBlockModel(SpecialBlockHelperData &helperRef, QTableView *parent)
    : QAbstractTableModel(parent), helper(helperRef), parent(parent), errorIcon(":/icon/execute/failed.png")
{
    Q_ASSERT(parent);
}

int SPRulePatternQuickInputSpecialBlockModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return table.size();
}

int SPRulePatternQuickInputSpecialBlockModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return NUM_COL;
}

QVariant SPRulePatternQuickInputSpecialBlockModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto& rowData = table.at(index.row());
    switch (index.column()) {
    default: return QVariant();
    case COL_Identifier: {
        switch (role) {
        default: return QVariant();
        case Qt::DecorationRole: {
            if (rowData.textHighlightRange.first >= 0) {
                return QVariant();
            } else {
                return QVariant(errorIcon);
            }
        }break;
        case Qt::DisplayRole: {
            QString text = rowData.identifier.first;
            int occurrenceIndex = rowData.identifier.second;
            if (occurrenceIndex != 0) {
                text.append(QString(" (#%1)").arg(occurrenceIndex + 1));
            }
            return QVariant(text);
        }break;
        }
    }break;
    case COL_ExportName: {
        switch (role) {
        default: return QVariant();
        case Qt::EditRole:
        case Qt::DisplayRole: {
            return QVariant(rowData.info.exportName);
        }break;
        }
    }break;
    case COL_Type: {
        switch (role) {
        default: return QVariant();
        case Qt::EditRole: {
            return QVariant(static_cast<int>(rowData.info.ty));
        }break;
        case Qt::DisplayRole: {
            return QVariant(getDisplayText(rowData.info.ty));
        }break;
        }
    }break;
    case COL_String: {
        switch (role) {
        default: return QVariant();
        case Qt::EditRole: {
            return QVariant(rowData.info.str);
        }break;
        case Qt::DisplayRole: {
            switch (rowData.info.ty) {
            default: break;
            case SpecialBlockType::AnonymousBoundary_Regex_Integer:
            case SpecialBlockType::AnonymousBoundary_Regex_Number:
            case SpecialBlockType::AnonymousBoundary_StringLiteral: {
                return QVariant(tr("(No Data)"));
            }break;
            }
            return QVariant(rowData.info.str);
        }break;
        }
    }break;
    }
}

QVariant SPRulePatternQuickInputSpecialBlockModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // no headers in vertical direction
    if (orientation == Qt::Vertical)
        return QVariant();

    switch (section) {
    default: return QVariant();
    case COL_Identifier: {
        switch (role) {
        default: return QVariant();
        case Qt::DisplayRole: {
            return QVariant(tr("Text"));
        }break;
        }
    }break;
    case COL_ExportName: {
        switch (role) {
        default: return QVariant();
        case Qt::DisplayRole: {
            return QVariant(tr("Export Name"));
        }break;
        }
    }break;
    case COL_Type: {
        switch (role) {
        default: return QVariant();
        case Qt::DisplayRole: {
            return QVariant(tr("Type"));
        }break;
        }
    }break;
    case COL_String: {
        switch (role) {
        default: return QVariant();
        case Qt::DisplayRole: {
            return QVariant(tr("Name / Expression"));
        }break;
        }
    }break;
    }
}

Qt::ItemFlags SPRulePatternQuickInputSpecialBlockModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    switch (index.column()) {
    case COL_Identifier: {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }break;
    case COL_ExportName: {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }break;
    case COL_Type: {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }break;
    case COL_String: {
        int row = index.row();
        const auto& rowData = table.at(row);
        switch (rowData.info.ty) {
        default: return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        case decltype (rowData.info.ty)::AnonymousBoundary_StringLiteral:
        case decltype (rowData.info.ty)::AnonymousBoundary_Regex_Integer:
        case decltype (rowData.info.ty)::AnonymousBoundary_Regex_Number:
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable; // not editable if it is a literal
        }
    }break;
    }

    return Qt::NoItemFlags;
}

bool SPRulePatternQuickInputSpecialBlockModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    int row = index.row();
    if (row < 0 || row >= table.size())
        return false;

    if (role != Qt::EditRole)
        return false;

    auto& rowData = table[row];
    bool isSetDirtyForLastField = false;
    switch (index.column()) {
    default: return false;
    case COL_ExportName: {
        rowData.info.exportName = value.toString();
    }break;
    case COL_Type: {
        rowData.info.ty = static_cast<SpecialBlockType>(value.toInt());
        isSetDirtyForLastField = true;
    }break;
    case COL_String: {
        rowData.info.str = value.toString();
    }break;
    }

    emit dataChanged(index, index);
    if (isSetDirtyForLastField) {
        QModelIndex lastFieldIndex = createIndex(index.row(), COL_String);
        emit dataChanged(lastFieldIndex, lastFieldIndex);
    }
    emit tableUpdated();
    return true;
}

QString SPRulePatternQuickInputSpecialBlockModel::getDisplayText(SpecialBlockType ty)
{
    switch (ty) {
    case SpecialBlockType::AnonymousBoundary_StringLiteral: return tr("String");
    case SpecialBlockType::AnonymousBoundary_Regex: return tr("Regular Expression");
    case SpecialBlockType::AnonymousBoundary_Regex_Integer: return tr("Integer");
    case SpecialBlockType::AnonymousBoundary_Regex_Number: return tr("Number");
    case SpecialBlockType::ContentType: return tr("Content");
    case SpecialBlockType::NamedBoundary: return tr("Named Mark");
    }
    qFatal("Unhandled SpecialBlockType");
    return QString();
}

void SPRulePatternQuickInputSpecialBlockModel::addBlockRequested(const SpecialBlockRecord& record)
{
    // step 1: find where to insert this record
    int textStart = record.textHighlightRange.first;
    int index = -1;
    for (int i = 0, n = table.size(); i < n; ++i) {
        const auto& row = table.at(i);
        if (row.textHighlightRange.first > textStart) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        // insert at the end of table
        index = table.size();
    }

    // step 2: actually do the insertion
    beginInsertRows(QModelIndex(), index, index);
    table.insert(index, record);
    endInsertRows();
    emit tableUpdated();

    // step 3: try refresh if adding this record invalidates others
    refreshTableForInvalidationCheck();
}

std::pair<int, int> SPRulePatternQuickInputSpecialBlockModel::findStringFromText(const QString& text, const QString& str, int occurrenceIndex)
{
    int startPos = 0;

    while (true) {
        int pos = text.indexOf(str, startPos);
        if (pos == -1) {
            return std::make_pair(-1, -1);
        }

        if (occurrenceIndex == 0) {
            return std::make_pair(pos, pos + str.length());
        }

        occurrenceIndex -= 1;
        startPos = pos + 1;
    }
}

void SPRulePatternQuickInputSpecialBlockModel::exampleTextUpdated(const QString& text)
{
    exampleText = text;
    refreshTableForInvalidationCheck();
}

void SPRulePatternQuickInputSpecialBlockModel::refreshTableForInvalidationCheck()
{
    const auto& curTable = table;
    struct RowInfo {
        std::pair<int, int> range;
        int index;
    };

    std::vector<RowInfo> rowInfo;
    rowInfo.reserve(curTable.size());

    // first pass: build rowInfo
    for (int i = 0, n = curTable.size(); i < n; ++i) {
        const auto& curIdentifier = curTable.at(i).identifier;
        RowInfo row;
        row.range = findStringFromText(exampleText, curIdentifier.first, curIdentifier.second);
        row.index = i;
        rowInfo.push_back(row);
    }

    // first sort on the rows
    auto rowOrder = [](const RowInfo& lhs, const RowInfo& rhs) -> bool {
        if (lhs.range.first < rhs.range.first)
            return true;
        if (lhs.range.first > rhs.range.first)
            return false;
        return lhs.index < rhs.index;
    };
    std::sort(rowInfo.begin(), rowInfo.end(), rowOrder);

    // check if any blocks is found overlapping; if yes, then remove the range of the one
    int tailPos = 0;
    for (int i = 0, n = rowInfo.size(); i < n; ++i) {
        auto& curRowInfo = rowInfo.at(i);
        if (curRowInfo.range.first < 0)
            continue;

        if (curRowInfo.range.first < tailPos) {
            // reject this block
            curRowInfo.range.first = -1;
            curRowInfo.range.second = -1;
        } else {
            // accept this one
            Q_ASSERT(curRowInfo.range.first < curRowInfo.range.second);
            tailPos = curRowInfo.range.second;
        }
    }

    // second sort
    std::sort(rowInfo.begin(), rowInfo.end(), rowOrder);
    Q_ASSERT(static_cast<int>(rowInfo.size()) == curTable.size());

    // populate the new table
    decltype(table) newTable;
    bool isNewTableDifferent = false;
    newTable.reserve(curTable.size());
    for (int i = 0, n = rowInfo.size(); i < n; ++i) {
        const RowInfo& info = rowInfo.at(i);
        const auto& originalEntry = curTable.at(info.index);
        auto newEntry = originalEntry;
        newEntry.textHighlightRange = info.range;
        newTable.push_back(newEntry);
        if (info.index != i || curTable.at(i).textHighlightRange != info.range) {
            isNewTableDifferent = true;
        }
    }

    if (!isNewTableDifferent)
        return;

    // replace the data
    beginResetModel();
    table.swap(newTable);
    endResetModel();
    emit tableUpdated();
}

void SPRulePatternQuickInputSpecialBlockModel::contextMenuHandler(const QPoint& pos)
{
    QModelIndex index = parent->indexAt(pos);

    // for now we only consider right clicks on the first column
    if (index.column() != 0)
        return;

    QMenu menu(parent);

    int rowIndex = -1;
    if (index.isValid()) {
        rowIndex = index.row();
    }
    if (rowIndex >= 0 && rowIndex < table.size()) {
        QAction* deleteAction = new QAction(tr("Delete"));
        connect(deleteAction, &QAction::triggered, this, [=](){
            beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
            table.removeAt(rowIndex);
            endRemoveRows();
            emit tableUpdated();
            refreshTableForInvalidationCheck();
        });
        menu.addAction(deleteAction);
    }

    if (!table.isEmpty()) {
        if (!menu.isEmpty()) {
            menu.addSeparator();
        }

        int invalidCount = 0;
        for(int i = 0, n = table.size(); i < n; ++i) {
            const auto& curRow = table.at(i);
            if (curRow.textHighlightRange.first < 0) {
                Q_ASSERT(invalidCount == i);
                invalidCount += 1;
            }
        }

        if (invalidCount > 0) {
            QAction* deleteInvalidAction = new QAction(tr("Delete invalid block(s)"));
            connect(deleteInvalidAction, &QAction::triggered, this, [=](){
                beginRemoveRows(QModelIndex(), 0, invalidCount-1);
                table.erase(table.begin(), table.begin() + invalidCount);
                endRemoveRows();
                // because we only remove invalid rows, this do not need to refresh table in any situation
                emit tableUpdated();
            });
            menu.addAction(deleteInvalidAction);
        }
    }

    if (menu.isEmpty())
        return;

    menu.exec(parent->mapToGlobal(pos));
}
