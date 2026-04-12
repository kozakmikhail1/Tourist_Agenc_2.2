#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include "containers/iterator.h"
#include <QVector>
#include <memory>
#include <algorithm>
#include <iterator>

template<typename T>
class DataContainer {
public:
    using Iterator = ContainerIterator<T>;
    using ConstIterator = ContainerIterator<const T>;

    DataContainer() = default;
    ~DataContainer() = default;

    void add(const T& item) {
        data_.append(item);
    }

    void remove(int index) {
        if (index >= 0 && index < data_.size()) {
            data_.removeAt(index);
        }
    }

    void remove(const T& item) {
        data_.removeOne(item);
    }

    T* get(int index) {
        if (index >= 0 && index < data_.size()) {
            return &data_[index];
        }
        return nullptr;
    }

    const T* get(int index) const {
        if (index >= 0 && index < data_.size()) {
            return &data_[index];
        }
        return nullptr;
    }

    int size() const { return data_.size(); }
    bool isEmpty() const { return data_.isEmpty(); }

    void clear() { data_.clear(); }

    Iterator begin() { return Iterator(data_.begin()); }
    Iterator end() { return Iterator(data_.end()); }

    ConstIterator begin() const { return ConstIterator(data_.begin()); }
    ConstIterator end() const { return ConstIterator(data_.end()); }

    ConstIterator cbegin() const { return ConstIterator(data_.begin()); }
    ConstIterator cend() const { return ConstIterator(data_.end()); }

    QVector<T>& getData() { return data_; }
    const QVector<T>& getData() const { return data_; }

    template<typename Predicate>
    Iterator findIf(Predicate pred) {
        auto it = std::find_if(data_.begin(), data_.end(), pred);
        return Iterator(it);
    }

    template<typename Predicate>
    ConstIterator findIf(Predicate pred) const {
        auto it = std::find_if(data_.begin(), data_.end(), pred);
        return ConstIterator(it);
    }
    
    template<typename Predicate>
    bool removeIf(Predicate pred) {
        auto it = std::find_if(data_.begin(), data_.end(), pred);
        if (it != data_.end()) {
            int index = std::distance(data_.begin(), it);
            data_.removeAt(index);
            return true;
        }
        return false;
    }

private:
    QVector<T> data_;
};

#endif



