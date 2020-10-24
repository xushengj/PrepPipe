#ifndef SPRULEPATTERNQUICKINPUTSPECIALBLOCKMODEL_H
#define SPRULEPATTERNQUICKINPUTSPECIALBLOCKMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QTableView>
#include <QIcon>

inline uint qHash(const std::pair<QString, int>& key, uint seed)
{
    return qHash(key.first, seed) + key.second;
}

class SPRulePatternQuickInputSpecialBlockModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class SpecialBlockType : int {
        AnonymousBoundary_StringLiteral,
        AnonymousBoundary_Regex,
        AnonymousBoundary_Regex_Integer,
        AnonymousBoundary_Regex_Number,
        ContentType,
        NamedBoundary
    };
    static QString getDisplayText(SpecialBlockType ty);
    struct SpecialBlockInfo {
        QString exportName;
        QString str;
        SpecialBlockType ty = SpecialBlockType::AnonymousBoundary_StringLiteral;
    };

    struct SpecialBlockHelperData {
        QHash<std::pair<QString, int>, SpecialBlockInfo> allExistingBlocks; // key: <str, occurrence index>; for auto-completion
    };

    struct SpecialBlockRecord {
        std::pair<QString, int> identifier;         // column 0, fixed
        std::pair<int, int> textHighlightRange;     // implicit
        SpecialBlockInfo info;                      // column 1-3: <export name, type, str>
    };

    enum TableColumn {
        COL_Identifier  = 0,
        COL_ExportName  = 1,
        COL_Type        = 2,
        COL_String      = 3,
        NUM_COL         = 4
    };

public:
    SPRulePatternQuickInputSpecialBlockModel(SpecialBlockHelperData& helperRef, QTableView* parent);
    const QList<SpecialBlockRecord>& getData() const {return table;}

public slots:
    void contextMenuHandler(const QPoint& pos);
    void addBlockRequested(const SpecialBlockRecord& record);
    void exampleTextUpdated(const QString& text);

signals:
    void tableUpdated();

public:
    static std::pair<int, int> findStringFromText(const QString& text, const QString& str, int occurrenceIndex);

private:
    void refreshTableForInvalidationCheck();

private:
    SpecialBlockHelperData& helper;
    QTableView* parent;

    // we guarantee that at any moment, the rows in the table are sorted by the text start position
    QList<SpecialBlockRecord> table;

    QIcon errorIcon;
    QString exampleText;

public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex& index = QModelIndex()) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
};

#endif // SPRULEPATTERNQUICKINPUTSPECIALBLOCKMODEL_H
