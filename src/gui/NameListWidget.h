#ifndef NAMELISTWIDGET_H
#define NAMELISTWIDGET_H

#include <QListWidget>
#include <QContextMenuEvent>
#include <QIcon>

class NameListWidget : public QListWidget
{
    Q_OBJECT
public:
    NameListWidget(QWidget* parent = nullptr);

    void setCheckNameCallback(std::function<bool(const QString&)> cb) {
        checkNameCB = cb;
    }

    void setData(const QStringList& dataArg);

    QStringList getData() {
        return data;
    }

    void setDefaultNewName(const QString& name) {
        defaultName = name;
    }

    void setNameDisplayName(const QString& name) {
        referredDisplayName = name;
    }

    // if true, right click menu on item would have a goto action that would result in emitting gotoRequested
    // default to false
    void setAcceptGotoRequest(bool isAcceptGoto) {
        isAcceptGotoRequest = isAcceptGoto;
    }

signals:
    void dirty();
    void gotoRequested(const QString& name);

private slots:
    void newRequested();
    void editRequested(QListWidgetItem* item);
    void deleteRequested(QListWidgetItem* item);

public slots:
    void nameCheckUpdateRequested();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    std::function<bool(const QString&)> checkNameCB;

    QIcon failIcon;
    QStringList data;
    QString defaultName; // what to use when creating new items
    QString referredDisplayName; // what is this "name"; used for gui

    bool isAcceptGotoRequest = false;
};

#endif // NAMELISTWIDGET_H
