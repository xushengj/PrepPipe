#include "Settings.h"

Settings* Settings::ptr = nullptr;

Settings::Settings() : QObject(nullptr)
{

}

void Settings::createInstance()
{
    Q_ASSERT(!ptr);
    ptr = new Settings;
    Q_ASSERT(ptr);
}

void Settings::destructInstance()
{
    Q_ASSERT(ptr);
    delete ptr;
    ptr = nullptr;
}

Settings* Settings::inst()
{
    return ptr;
}

void Settings::setValue(const QString& key, const QVariant& value)
{
    s.setValue(key, value);
    emit settingChanged(QStringList() << key);
}

void Settings::setValue(const QStringList& keyList, const QVariantList &valueList)
{
    Q_ASSERT(keyList.size() == valueList.size());
    for (int i = 0, n = keyList.size(); i < n; ++i) {
        s.setValue(keyList.at(i), valueList.at(i));
    }
    emit settingChanged(keyList);
}
