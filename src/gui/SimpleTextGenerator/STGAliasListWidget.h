#ifndef STGALIASLISTWIDGET_H
#define STGALIASLISTWIDGET_H

#include <QListWidget>
#include <QContextMenuEvent>

class STGAliasListWidget : public QListWidget
{
    Q_OBJECT
public:
    STGAliasListWidget(QWidget* parent = nullptr);

    void setCanonicalName(const QString& canonicalName) {
        cname = canonicalName;
    }
    void setData(const QStringList& alias);

    void setUnbacked();

    QStringList getData();

    QListWidgetItem *addAlias(const QString& str);

signals:
    void dirty();

private slots:
    void newAliasRequested(const QString& initialName);
    void editAliasRequested(QListWidgetItem* item);
    void deleteAliasRequested(QListWidgetItem* item);

    void flagDirty() {
        isDirty = true;
        emit dirty();
    }

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QStringList originalData;
    QString cname;
    bool isDirty = false;
};

#endif // STGALIASLISTWIDGET_H
