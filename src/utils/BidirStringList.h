#ifndef BIDIRSTRINGLIST_H
#define BIDIRSTRINGLIST_H

#include <QHash>
#include <QStringList>

#include "src/GlobalInclude.h"
#include "src/utils/NameSorting.h"

class BidirStringList {
public:
    BidirStringList() = default;
    BidirStringList(const BidirStringList&) = default;
    BidirStringList(BidirStringList&&) = default;
    BidirStringList(const QStringList& listArg)
        : list(listArg)
    {
        rebuildHash();
    }
    BidirStringList& operator=(const BidirStringList&) = default;
    BidirStringList& operator=(BidirStringList&&) = default;
    BidirStringList& operator=(const QStringList& listArg) {
        list = listArg;
        rebuildHash();
        return *this;
    }

    const QString& at(indextype idx) const {return list.at(idx);}
    indextype size() const {return list.size();}
    bool empty() const {return list.isEmpty();}
    bool isEmpty() const {return list.isEmpty();}
    int indexOf(const QString& str) const {return hash.value(str, -1);}
    bool contains(const QString& str) const {return hash.contains(str);}
    const QStringList& getList() const {return list;}
    void push_back(const QString& str) {
        Q_ASSERT(!hash.contains(str));
        indextype idx = list.size();
        list.push_back(str);
        hash.insert(str, idx);
    }
    void clear() {
        list.clear();
        hash.clear();
    }
    void sort(Qt::CaseSensitivity cs = Qt::CaseSensitive) {
        list.sort(cs);
        rebuildHash();
    }
    void sortWithCollator() {
        NameSorting::sortNameList(list);
        rebuildHash();
    }
    void removeAt(indextype idx) {
        Q_ASSERT(idx >= 0 && idx < list.size());
        list.removeAt(idx);
        rebuildHash();
    }
    void reserve(indextype size) {
        list.reserve(size);
        hash.reserve(size);
    }

private:
    void rebuildHash() {
        hash.clear();
        hash.reserve(list.size());
        for (indextype i = 0, n = list.size(); i < n; ++i) {
            Q_ASSERT(!hash.contains(list.at(i)));
            hash.insert(list.at(i), i);
        }
    }

private:
    QHash<QString, indextype> hash;
    QStringList list;
};

#endif // BIDIRSTRINGLIST_H
