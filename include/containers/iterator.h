#ifndef ITERATOR_H
#define ITERATOR_H

#include <iterator>
#include <QVector>

// Кастомный итератор для контейнера
// Демонстрирует: перегрузку операций для итераторов
template<typename T>
class ContainerIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    ContainerIterator() : ptr_(nullptr) {}
    explicit ContainerIterator(typename QVector<T>::iterator it) : ptr_(it) {}

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    ContainerIterator& operator++() {
        ++ptr_;
        return *this;
    }

    ContainerIterator operator++(int) {
        ContainerIterator tmp = *this;
        ++ptr_;
        return tmp;
    }

    ContainerIterator& operator--() {
        --ptr_;
        return *this;
    }

    ContainerIterator operator--(int) {
        ContainerIterator tmp = *this;
        --ptr_;
        return tmp;
    }

    ContainerIterator& operator+=(difference_type n) {
        ptr_ += n;
        return *this;
    }

    ContainerIterator& operator-=(difference_type n) {
        ptr_ -= n;
        return *this;
    }

    friend ContainerIterator operator+(difference_type n, const ContainerIterator& it) {
        ContainerIterator tmp = it;
        return tmp += n;
    }

    friend ContainerIterator operator+(const ContainerIterator& it, difference_type n) {
        ContainerIterator tmp = it;
        return tmp += n;
    }

    friend ContainerIterator operator-(const ContainerIterator& it, difference_type n) {
        ContainerIterator tmp = it;
        return tmp -= n;
    }

    friend difference_type operator-(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ - rhs.ptr_;
    }

    friend bool operator==(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ == rhs.ptr_;
    }

    friend bool operator!=(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ != rhs.ptr_;
    }

    friend bool operator<(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ < rhs.ptr_;
    }

    friend bool operator>(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ > rhs.ptr_;
    }

    friend bool operator<=(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ <= rhs.ptr_;
    }

    friend bool operator>=(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ >= rhs.ptr_;
    }

private:
    typename QVector<T>::iterator ptr_;
};

// Специализация для const итератора - использует общие операторы через наследование
template<typename T>
class ContainerIterator<const T> {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    ContainerIterator() : ptr_(nullptr) {}
    explicit ContainerIterator(typename QVector<T>::const_iterator it) : ptr_(it) {}

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    ContainerIterator& operator++() { ++ptr_; return *this; }
    ContainerIterator operator++(int) { ContainerIterator tmp = *this; ++ptr_; return tmp; }
    ContainerIterator& operator--() { --ptr_; return *this; }
    ContainerIterator operator--(int) { ContainerIterator tmp = *this; --ptr_; return tmp; }
    ContainerIterator& operator+=(difference_type n) { ptr_ += n; return *this; }
    ContainerIterator& operator-=(difference_type n) { ptr_ -= n; return *this; }

    friend ContainerIterator operator+(difference_type n, const ContainerIterator& it) {
        ContainerIterator tmp = it;
        return tmp += n;
    }

    friend ContainerIterator operator+(const ContainerIterator& it, difference_type n) {
        ContainerIterator tmp = it;
        return tmp += n;
    }

    friend ContainerIterator operator-(const ContainerIterator& it, difference_type n) {
        ContainerIterator tmp = it;
        return tmp -= n;
    }

    friend difference_type operator-(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ - rhs.ptr_;
    }

    friend bool operator==(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ == rhs.ptr_;
    }

    friend bool operator!=(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ != rhs.ptr_;
    }

    friend bool operator<(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ < rhs.ptr_;
    }

    friend bool operator>(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ > rhs.ptr_;
    }

    friend bool operator<=(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ <= rhs.ptr_;
    }

    friend bool operator>=(const ContainerIterator& lhs, const ContainerIterator& rhs) {
        return lhs.ptr_ >= rhs.ptr_;
    }

private:
    typename QVector<T>::const_iterator ptr_;
};

#endif // ITERATOR_H














