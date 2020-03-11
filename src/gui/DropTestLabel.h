#ifndef DROPTESTLABEL_H
#define DROPTESTLABEL_H

#include <QObject>
#include <QLabel>
#include <QMimeData>

class DropTestLabel : public QLabel
{
    Q_OBJECT

public:
    DropTestLabel(const QString &text, QWidget *parent = nullptr);
    DropTestLabel(QWidget *parent = nullptr);
    virtual ~DropTestLabel() override;

signals:
    void dataDropped(const QMimeData* data);

protected:
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
};

#endif // DROPTESTLABEL_H
