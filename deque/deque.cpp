#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <random>
#include <type_traits>
#include <unordered_set>

template<typename T>
class Deque {
private:
    T **outer_array_ = nullptr;
    size_t outer_size_ = 0;
    size_t size_ = 0;
    static const size_t kSizes = 16;

    template<bool is_const>
    class random_iterator {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef T value_type;
        typedef int difference_type;

        typedef typename std::conditional<is_const, const T &, T &>::type reference;
        typedef typename std::conditional<is_const, const T *, T *>::type pointer;
        typedef typename std::conditional<is_const, const T **, T **>::type outer_ptr;

        friend class Deque;

        T **iter_object_ = nullptr;
        size_t outer_ = 0;
        size_t inter_ = 0;
        size_t rows_count = 0;
        T *current_row = nullptr;

        random_iterator() {}

        random_iterator(T **iter_object, size_t outer, size_t inter, size_t rows_count);

        template<typename Iterator>
        requires(std::is_same_v<Deque<T>::random_iterator<is_const>,
                Deque<T>::random_iterator<true>> &&
                 std::is_same_v<Iterator, Deque<T>::random_iterator<false>>)
        random_iterator<is_const> &operator=(const Iterator &other) {
            random_iterator<is_const> copy(other);
            std::swap(other.iter_object_, iter_object_);
            std::swap(inter_, other.inter_);
            std::swap(outer_, other.outer_);
            std::swap(rows_count, other.rows_count);
            return *this;
        }

        template<typename Iterator>
        requires(std::is_same_v<random_iterator<is_const>,
                random_iterator<true>> &&
                 std::is_same_v<Iterator, random_iterator<false>>)
        random_iterator(const Iterator &it)
                : iter_object_(it.iter_object_), outer_(it.outer_), inter_(it.inter_), rows_count(it.rows_count),
                  current_row(it.current_row) {}

        random_iterator<is_const> &operator++();

        random_iterator<is_const> &operator--();

        random_iterator<is_const> operator++(int);

        random_iterator<is_const> operator--(int);

        random_iterator<is_const> &operator+=(int num);

        random_iterator<is_const> &operator-=(int num);

        random_iterator<is_const> operator+(int num) const;

        random_iterator<is_const> operator-(int num) const;

        template<bool annother_is_const>
        int operator-(const random_iterator<annother_is_const> &another);

        reference operator*() const noexcept {
            return current_row[inter_];
        }

        pointer operator->() const noexcept {
            return &current_row[inter_];
        }

        bool operator<(const random_iterator<is_const> &another) const;

        bool operator>(const random_iterator<is_const> &another) const;

        bool operator<=(const random_iterator<is_const> &another) const;

        bool operator>=(const random_iterator<is_const> &another) const;

        bool operator==(const random_iterator<is_const> &another) const;

        bool operator!=(const random_iterator<is_const> &another) const;
    };

public:
    using iterator = random_iterator<false>;
    using cst_iterator = random_iterator<true>;

    using reverse_iterator = std::reverse_iterator<random_iterator<false>>;
    using const_reverse_iterator = std::reverse_iterator<random_iterator<true>>;

private:
    void outer_resize(size_t free_prefix_size, size_t free_suffix_size);

    void outer_safe_remove(T **&array);

    iterator begin_;
    iterator end_;

public:
    Deque() {};

    Deque(const Deque &deque);

    Deque(size_t num);

    Deque(size_t num, const T &elem);

    Deque<T> &operator=(const Deque<T> &another);

    size_t size() const;

    T &operator[](size_t index);

    const T &operator[](size_t index) const;

    T &at(size_t index);

    const T &at(size_t index) const;

    void push_back(const T &new_element);

    void push_front(const T &new_element);

    void pop_back();

    void pop_front();

    void insert(iterator it, const T &object);

    void erase(iterator it);

    iterator begin();

    iterator end();

    cst_iterator begin() const;

    cst_iterator end() const;

    cst_iterator cbegin() const;

    cst_iterator cend() const;

    reverse_iterator rbegin();

    reverse_iterator rend();

    const_reverse_iterator rbegin() const;

    const_reverse_iterator rend() const;

    const_reverse_iterator crbegin() const;

    const_reverse_iterator crend() const;
};

template<typename T>
Deque<T>::Deque(size_t num, const T &kElem) {
    size_ = num;
    size_t need_arrays = (num + kSizes) / kSizes;
    Deque<T>::outer_resize(0, need_arrays);
    begin_ = iterator(outer_array_, 0, 0, outer_size_);
    size_t index = 0;
    try {
        for (; index < size_; ++index) {
            new(&((*this)[index])) T(kElem);

        }
    } catch (...) {
        for (size_t i = 0; i < index; ++i) {
            (*this)[index].~T();
        }
        for (size_t i = 0; i < outer_size_; ++i) {
            delete[] reinterpret_cast<char *>(outer_array_[i]);
        }
        delete[] reinterpret_cast<char *>(outer_array_);
    }
    end_ = begin_ + size_;
}

template<typename T>
Deque<T>::Deque(size_t num) {
    outer_resize(0, (num + kSizes - 1) / kSizes);
    size_ = num;
    begin_ = iterator(outer_array_, 0, 0, outer_size_);
    end_ = begin_ + size_;
    if constexpr (std::is_default_constructible_v<T>) {
        size_t index = 0;
        try {
            for (; index < size_; ++index) {
                new(&((*this)[index])) T();
            }
        } catch (...) {
            for (size_t i = 0; i < index; ++i) {
                (*this)[i].~T();
            }
        }
    }
}

template<typename T>
Deque<T> &Deque<T>::operator=(const Deque<T> &another) {
    if (this == &another) {
        return *this;
    }

    Deque<T> copy(another);
    std::swap(outer_array_, copy.outer_array_);
    std::swap(outer_size_, copy.outer_size_);
    std::swap(size_, copy.size_);
    std::swap(begin_, copy.begin_);
    std::swap(end_, copy.end_);

    return *this;
}

template<typename T>
Deque<T>::Deque(const Deque &deque) {
    try {
        outer_array_ =
                reinterpret_cast<T **>(new char[deque.outer_size_ * sizeof(T *)]);
    } catch (...) {
        delete[] reinterpret_cast<char *>(outer_array_);
        return;
    }
    size_t index1 = 0;
    try {
        while (index1 < deque.outer_size_) {
            outer_array_[index1] = reinterpret_cast<T *>(new char[kSizes * sizeof(T)]);
            ++index1;
        }
    } catch (...) {
        for (size_t i = 0; i < index1; ++i) {
            delete[] reinterpret_cast<char *>(outer_array_[index1]);
        }
        delete[] reinterpret_cast<char *>(outer_array_);
        return;
    }
    auto it = deque.begin_;
    try {
        while (it < deque.end_) {
            new(outer_array_[it.outer_] + it.inter_) T(*it);
            ++it;
        }
    } catch (...) {
        while (it > deque.begin_) {
            outer_array_[it.outer_][it.inter_].~T();
            --it;
        }
        for (size_t i = 0; i < deque.outer_size_; ++i) {
            delete[] reinterpret_cast<char *>(outer_array_[i]);
        }
        delete[] reinterpret_cast<char *>(outer_array_);
    }

    outer_size_ = deque.outer_size_;
    size_ = deque.size_;
    begin_ = iterator(outer_array_, deque.begin_.outer_, deque.begin_.inter_, outer_size_);
    end_ = begin_ + size_;
}

template<typename T>
void Deque<T>::outer_resize(size_t free_prefix_size, size_t free_suffix_size) {
    size_t new_size = (free_prefix_size + outer_size_ + free_suffix_size);
    size_t index = 0;
    T **new_outer_array = nullptr;
    new_outer_array = reinterpret_cast<T **>(new char[new_size * sizeof(T *)]);

    while (index < new_size) {
        if (index >= free_prefix_size && index < free_prefix_size + outer_size_) {
            new_outer_array[index] = outer_array_[index - free_prefix_size];
        } else {
            new_outer_array[index] =
                    reinterpret_cast<T *>(new char[kSizes * sizeof(T)]);
        }
        ++index;
    }

    delete[] outer_array_;

    outer_array_ = new_outer_array;
    outer_size_ = new_size;

    begin_ =
            iterator(outer_array_, free_prefix_size + begin_.outer_, begin_.inter_, outer_size_);
    end_ = begin_ + size_;
}

template<typename T>
void Deque<T>::insert(iterator it, const T &object) {
    T copy(object);
    for (; it < end(); ++it) {
        std::swap(copy, *it);
    }
    try {
        push_back(copy);
    } catch (...) {
        for (auto iter = end_ - 1; iter != begin_; --iter) {
            std::swap(copy, *it);
        }
        throw;
    }

}

template<typename T>
void Deque<T>::erase(iterator it) {
    for (; (it + 1) < end(); ++it) {
        std::swap(*(it + 1), *it);
    }
    pop_back();
}

template<typename T>
const T &Deque<T>::at(size_t index) const {
    if (index < 0 || index >= size_) {
        throw std::out_of_range("deque index out of range");
    }
    return *(begin_ + index);
}

template<typename T>
void Deque<T>::pop_front() {
    --size_;
    if (size_ == 0) {
        --end_;
        return;
    }
    ++begin_;
}

template<typename T>
void Deque<T>::pop_back() {
    --size_;
    --end_;
}

template<typename T>
void Deque<T>::push_front(const T &new_element) {
    if (size_ == 0) {
        push_back(new_element);
        return;
    }
    if (begin_.outer_ == 0 && begin_.inter_ == 0) {
        outer_resize(outer_size_, 0);
    }
    --begin_;
    try {
        new(outer_array_[begin_.outer_] + begin_.inter_) T(new_element);
    } catch (...) {
        ++begin_;
        throw;
    }
    ++size_;
}

template<typename T>
void Deque<T>::push_back(const T &new_element) {
    if (end_.outer_ == outer_size_) {
        outer_resize(0, std::max(outer_size_, (size_t) 1));
    }
    new(outer_array_[end_.outer_] + end_.inter_) T(new_element);
    ++size_;
    ++end_;
}

template<typename T>
const T &Deque<T>::operator[](size_t index) const {
    return *(begin_ + index);
}

template<typename T>
T &Deque<T>::operator[](size_t index) {
    return *(begin_ + index);
}

template<typename T>
T &Deque<T>::at(size_t index) {
    if (index < 0 || index >= size_) {
        throw std::out_of_range("deque index out of range");
    }
    return *(begin_ + index);
}

template<typename T>
size_t Deque<T>::size() const {
    return size_;
}

// ITERATORS

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const>
Deque<T>::random_iterator<is_const>::operator+(int num) const {
    Deque<T>::random_iterator<is_const> copy(*this);
    return copy += num;
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const>
Deque<T>::random_iterator<is_const>::operator-(int num) const {
    Deque<T>::random_iterator<is_const> copy(*this);
    return copy -= num;
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const>::random_iterator(T **iter_object,
                                                     size_t outer, size_t inter, size_t rows_count)
        : iter_object_(const_cast<outer_ptr>(iter_object)),
          outer_(outer),
          inter_(inter), rows_count(rows_count) {
    if (outer_ < rows_count) {
        current_row = iter_object[outer_];
    }
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const> &
Deque<T>::random_iterator<is_const>::operator++() {
    if (inter_ == (kSizes - 1)) {
        ++outer_;
        inter_ = -1;
        if (outer_ < rows_count) {
            current_row = iter_object_[outer_];
        } else {
            current_row = nullptr;
        }
    }
    ++inter_;
    return *this;
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const>
Deque<T>::random_iterator<is_const>::operator++(int) {
    Deque<T>::random_iterator<is_const> copy(*this);
    ++(*this);
    return copy;
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const> &
Deque<T>::random_iterator<is_const>::operator--() {
    if (inter_ == 0) {
        --outer_;
        inter_ = kSizes;
        if (0 <= outer_ < rows_count) {
            current_row = iter_object_[outer_];
        } else {
            current_row = nullptr;
        }
    }
    --inter_;
    return *this;
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const>
Deque<T>::random_iterator<is_const>::operator--(int) {
    Deque<T>::random_iterator<is_const> copy(*this);
    ++(*this);
    return copy;
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const> &
Deque<T>::random_iterator<is_const>::operator+=(int num) {
    if (num < 0) {
        return (*this -= (-num));
    }
    outer_ += (num + inter_) / kSizes;
    inter_ = (inter_ + num % kSizes) % kSizes;
    if (outer_ < rows_count) {
        current_row = iter_object_[outer_];
    } else {
        current_row = nullptr;
    }
    return *this;
}

template<typename T>
template<bool is_const>
Deque<T>::random_iterator<is_const> &
Deque<T>::random_iterator<is_const>::operator-=(int num) {
    if (num < 0) {
        return ((*this) += (-num));
    }
    outer_ = (outer_ * kSizes + inter_ - num) / kSizes;
    inter_ = (inter_ + kSizes - num % kSizes) % kSizes;
    if (0 <= outer_ <= rows_count) {
        current_row = iter_object_[outer_];
    } else {
        current_row = nullptr;
    }
    return *this;
}

template<typename T>
template<bool is_const>
bool Deque<T>::random_iterator<is_const>::operator<(
        const Deque<T>::random_iterator<is_const> &another) const {
    if (outer_ == another.outer_) {
        return inter_ < another.inter_;
    }
    return outer_ < another.outer_;
}

template<typename T>
template<bool is_const>
bool Deque<T>::random_iterator<is_const>::operator>(
        const Deque<T>::random_iterator<is_const> &another) const {
    return another < *this;
}

template<typename T>
template<bool is_const>
bool Deque<T>::random_iterator<is_const>::operator<=(
        const Deque<T>::random_iterator<is_const> &another) const {
    return !(*this > another);
}

template<typename T>
template<bool is_const>
bool Deque<T>::random_iterator<is_const>::operator!=(
        const Deque<T>::random_iterator<is_const> &another) const {
    return !(*this == another);
}

template<typename T>
template<bool is_const>
bool Deque<T>::random_iterator<is_const>::operator==(
        const Deque<T>::random_iterator<is_const> &another) const {
    return outer_ == another.outer_ && inter_ == another.inter_;
}

template<typename T>
template<bool is_const>
bool Deque<T>::random_iterator<is_const>::operator>=(
        const Deque<T>::random_iterator<is_const> &another) const {
    return !(*this < another);
}

template<typename T>
Deque<T>::iterator Deque<T>::begin() {
    return Deque::iterator(begin_);
}

template<typename T>
Deque<T>::iterator Deque<T>::end() {
    return Deque::iterator(end_);
}

template<typename T>
Deque<T>::cst_iterator Deque<T>::begin() const {
    return cbegin();
}

template<typename T>
Deque<T>::cst_iterator Deque<T>::end() const {
    return cend();
}

template<typename T>
Deque<T>::cst_iterator Deque<T>::cbegin() const {
    return Deque::cst_iterator(begin_);
}

template<typename T>
Deque<T>::cst_iterator Deque<T>::cend() const {
    return Deque::cst_iterator(end_);
}

template<typename T>
Deque<T>::reverse_iterator Deque<T>::rbegin() {
    return Deque::reverse_iterator(end_);
}

template<typename T>
Deque<T>::reverse_iterator Deque<T>::rend() {
    return Deque::reverse_iterator(begin_);
}

template<typename T>
Deque<T>::const_reverse_iterator Deque<T>::rbegin() const {
    return crbegin();
}

template<typename T>
Deque<T>::const_reverse_iterator Deque<T>::rend() const {
    return crend();
}

template<typename T>
Deque<T>::const_reverse_iterator Deque<T>::crbegin() const {
    return Deque::const_reverse_iterator(end_);
}

template<typename T>
Deque<T>::const_reverse_iterator Deque<T>::crend() const {
    return Deque::const_reverse_iterator(begin_);
}

template<typename T>
template<bool is_const>
template<bool annother_is_const>
int Deque<T>::random_iterator<is_const>::operator-(
        const Deque::random_iterator<annother_is_const> &another) {
    return kSizes * (outer_ - another.outer_) + inter_ - another.inter_;
}
