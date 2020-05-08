#ifndef NAMESORTING_H
#define NAMESORTING_H

#include <QCollator>
#include <algorithm>
#include <utility>

class NameSorting
{
public:
    NameSorting() = delete;

    static void init();

    static void sortNameList(QStringList& list)
    {
        std::sort(list.begin(), list.end(), [=](const QString& lhs, const QString& rhs) -> bool {
            return collator->compare(lhs, rhs) < 0;
        });
    }

    template <typename ValueTy>
    static void sortNameListWithData(QVector<std::pair<QString, ValueTy>>& list)
    {
        std::sort(list.begin(), list.end(), [=](const std::pair<QString, ValueTy>& lhs, const std::pair<QString, ValueTy>& rhs) -> bool {
            return collator->compare(lhs.first, rhs.first) < 0;
        });
    }

private:
    static QCollator* collator;
};

#endif // NAMESORTING_H
