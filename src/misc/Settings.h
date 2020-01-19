#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings* inst();

    static void createInstance();
    static void destructInstance();

    bool contains(const QString& key) const {
        return s.contains(key);
    }

    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const {
        return s.value(key, defaultValue);
    }

signals:
    void settingChanged(const QStringList& keyList);

public slots:
    void setValue(const QString& key, const QVariant& value);
    void setValue(const QStringList& keyList, const QVariantList& valueList);

private:
    QSettings s;

    explicit Settings();
    static Settings* ptr;
};

#endif // SETTINGS_H
