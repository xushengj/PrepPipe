#ifndef CONTIGUOUSINDEXVECTOR_H
#define CONTIGUOUSINDEXVECTOR_H

#include "src/GlobalInclude.h"

#include <type_traits>
#include <iterator>

/**
 * ContiguousIndexVector: replacement for QVector<indextype> when indices are guaranteed to be contiguous
 *
 * Basically this class pretends to be a QVector<indextype> while it is really just a pair of indextype that represent the range
 */
template <typename IndexType = indextype>
class ContiguousIndexVector
{
    static_assert(std::is_integral<IndexType>::value, "Integer type required for IndexType");
public:
    // note: this is only used for range-based for loop
    class const_iterator : public std::iterator<std::random_access_iterator_tag, const IndexType, IndexType> {
    public:
        const_iterator() : val(0) {}
        explicit const_iterator(IndexType idx) : val(idx) {}
        const_iterator& operator++() {++val; return *this;}
        const_iterator operator++(int) {const_iterator retval = *this; ++(*this); return retval;}
        const_iterator& operator--() {--val; return *this;}
        const_iterator operator--(int) {const_iterator retval = *this; --(*this); return retval;}
        const_iterator operator+(IndexType off) const {
            return const_iterator(val + off);
        }
        const_iterator operator-(IndexType off) const {
            return const_iterator(val - off);
        }
        const_iterator& operator+=(IndexType off) {val += off; return *this;}
        const_iterator& operator-=(IndexType off) {val -= off; return *this;}
        //friend const_iterator operator+(indextype, const_iterator);
        //friend const_iterator operator-(indextype, const_iterator);
        IndexType operator-(const_iterator other) const {
            return val - other.val;
        }
        bool operator==(const_iterator other) const {return val == other.val;}
        bool operator!=(const_iterator other) const {return !(*this == other);}
        bool operator<(const_iterator other) const {return val < other.val;}
        bool operator>(const_iterator other) const {return val > other.val;}
        bool operator<=(const_iterator other) const {return val <= other.val;}
        bool operator>=(const_iterator other) const {return val >= other.val;}
        IndexType operator*() const {return val;}
        IndexType operator[](IndexType off) const {return val+off;}
    private:
        IndexType val;
    };

public:
    ContiguousIndexVector() = default;
    ContiguousIndexVector(const ContiguousIndexVector&) = default;
    ContiguousIndexVector(ContiguousIndexVector&&) = default;
    ContiguousIndexVector(IndexType lowerBound, IndexType upperBound)
        : lb(lowerBound), ub(upperBound)
    {
        Q_ASSERT(lb <= ub);
    }

    ContiguousIndexVector& operator=(const ContiguousIndexVector&) = default;
    ContiguousIndexVector& operator=(ContiguousIndexVector&&) = default;

    const_iterator begin() const {return const_iterator(lb);}
    const_iterator end() const {return const_iterator(ub);}
    indextype indexOf(IndexType idx) const {return static_cast<indextype>(idx - lb);}
    IndexType at(indextype idx) const {return lb + idx;}
    indextype size() const {return static_cast<indextype>(ub - lb);}
    bool empty() const {return ub == lb;}
    bool isEmpty() const {return ub == lb;}
    indextype front() const {Q_ASSERT(!empty()); return lb;}
    indextype back() const {Q_ASSERT(!empty()); return ub-1;}

    void push_back(IndexType idx) {
        if (lb == ub) {
            lb = idx;
            ub = idx+1;
        } else {
            Q_ASSERT(ub == idx);
            ub += 1;
        }
    }
    void reserve(indextype size) {
        Q_UNUSED(size);
    }
    void clear() {
        lb = 0;
        ub = 0;
    }
private:
    IndexType lb = 0;
    IndexType ub = 0;
};

#endif // CONTIGUOUSINDEXVECTOR_H
