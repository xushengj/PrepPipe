#ifndef NAMESORTING_H
#define NAMESORTING_H

#include <QCollator>
#include <algorithm>
#include <utility>

class NameSorting
{
public:
    NameSorting() = delete;

    static void init()
    {
        collator = new QCollator(QLocale());
    }

    static void sortNameList(QStringList& list)
    {
        if (Q_UNLIKELY(!collator)) {
            init();
        }
        std::sort(list.begin(), list.end(), [=](const QString& lhs, const QString& rhs) -> bool {
            return collator->compare(lhs, rhs) < 0;
        });
    }

    template <typename ValueTy>
    static void sortNameListWithData(QVector<std::pair<QString, ValueTy>>& list)
    {
        if (Q_UNLIKELY(!collator)) {
            init();
        }
        std::sort(list.begin(), list.end(), [=](const std::pair<QString, ValueTy>& lhs, const std::pair<QString, ValueTy>& rhs) -> bool {
            return collator->compare(lhs.first, rhs.first) < 0;
        });
    }

private:
    // defined in src/utils/NameSorting.cpp
    static QCollator* collator;
};

#endif // NAMESORTING_H
