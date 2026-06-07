#include <iostream>
#include <type_traits>

template<typename T, size_t N>
class StackAllocator;

template<typename T, typename Alloc = std::allocator<T>>
class List {
private:
    struct BaseNode {
        BaseNode *prev = nullptr;
        BaseNode *next = nullptr;

        BaseNode() {}

        BaseNode(const BaseNode &base_node)
                : prev(base_node.prev), next(base_node.next) {}

        BaseNode(BaseNode *prev, BaseNode *next) : prev(prev), next(next) {}
    };

    struct Node : BaseNode {
        T value;

        Node() : BaseNode() {}

        Node(BaseNode *prev, BaseNode *next, const T &value)
                : BaseNode(prev, next), value(value) {}

        Node(const BaseNode &node, const T &value) : BaseNode(node), value(value) {}

        Node(BaseNode *prev, BaseNode *next) : BaseNode(prev, next) {}

        Node(const Node &another)
                : BaseNode(another.prev, another.next), value(another.value) {
            ;
        }
    };

    template<bool is_const>
    struct BaseIter {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = int;
        using value_type = T;

        using reference = typename std::conditional<is_const, const T &, T &>::type;
        using pointer = typename std::conditional<is_const, const T *, T *>::type;
        using base_pointer =
                typename std::conditional<is_const, const BaseNode *, BaseNode *>::type;
        using node_pointer =
                typename std::conditional<is_const, const Node *, Node *>::type;

        BaseNode *iterator_;

        BaseIter<is_const> &operator++() {
            iterator_ = iterator_->next;
            return *this;
        }

        BaseIter<is_const> &operator--() {
            iterator_ = iterator_->prev;
            return *this;
        }

        BaseIter<is_const> operator++(int) {
            BaseIter<is_const> old_iterator = *this;
            iterator_ = iterator_->next;
            return old_iterator;
        }

        BaseIter<is_const> operator--(int) {
            BaseIter<is_const> old_iterator = *this;
            iterator_ = iterator_->prev;
            return old_iterator;
        }

        template<bool another_is_const>
        bool operator==(const BaseIter<another_is_const> &another) {
            return iterator_ == another.iterator_;
        }

        template<bool another_is_const>
        bool operator!=(const BaseIter<another_is_const> &another) {
            return iterator_ != another.iterator_;
        }

        reference operator*() const noexcept {
            return static_cast<node_pointer>(iterator_)->value;
        }

        pointer operator->() const noexcept {
            return &static_cast<node_pointer>(iterator_)->value;;
        }

        template<bool another_is_const>
        requires(std::is_same_v<BaseIter<is_const>, BaseIter<true>> &&
                 std::is_same_v<BaseIter<another_is_const>, BaseIter<false>>)
        BaseIter<is_const> &operator=(const BaseIter<another_is_const> &other) {
            iterator_ = other.iterator_;
            return *this;
        }

        template<bool another_is_const>
        requires(std::is_same_v<BaseIter<is_const>, BaseIter<true>> &&
                 std::is_same_v<BaseIter<another_is_const>, BaseIter<false>>)
        BaseIter(const BaseIter<another_is_const> &it) : iterator_(it.iterator_) {}

        BaseIter(base_pointer node) : iterator_(node->next->prev) {}

        bool operator!=(const BaseIter<is_const> &another) const {
            return !(*this == another);
        }

        bool operator==(const BaseIter<is_const> &another) const {
            return iterator_ == another.iterator_;
        }
    };

public:
    using NodeAlloc =
            typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    using AllocTraits = std::allocator_traits<NodeAlloc>;

    using iterator = BaseIter<false>;
    using const_iterator = BaseIter<true>;

    using reverse_iterator = std::reverse_iterator<BaseIter<false>>;
    using const_reverse_iterator = std::reverse_iterator<BaseIter<true>>;

private:
    BaseNode fakeNode_;
    size_t size_;
    [[no_unique_address]] NodeAlloc alloc_;

public:
    List() : fakeNode_(&fakeNode_, &fakeNode_), size_(0) {}

    List(size_t count, const T &elem, const Alloc &allocator = Alloc())
            : List(allocator) {
        size_t index = 0;
        try {
            for (; index < count; ++index) {
                push_back(elem);
            }
        } catch (...) {
            for (size_t i = 0; i < index; ++i) {
                pop_back();
            }
        }
    }

    List(const Alloc &allocator) : List() {
        alloc_ = AllocTraits::select_on_container_copy_construction(allocator);
    }

    List(size_t count, const Alloc &allocator = Alloc()) : List(allocator) {
        size_ = count;
        Node *tmp = static_cast<Node *>(&fakeNode_);
        size_t index = 0;
        try {
            Node *newNode = nullptr;
            for (; index < count; ++index) {
                newNode = AllocTraits::allocate(alloc_, 1);
                AllocTraits::construct(alloc_, newNode, static_cast<BaseNode *>(tmp),
                                       &fakeNode_);
                tmp->next = newNode;
                tmp = newNode;
            }
            fakeNode_.prev = newNode;
        } catch (...) {
            for (; index != 0; --index) {
                pop_back();
            }
        }

    }

    List(const List<T, Alloc> &another_list)
            : List(another_list.get_allocator()) {
        try {
            for (auto &node: another_list) {
                push_back(node);
            }
        } catch (...) {
            Clear();
            throw;
        }
    }

    void Clear() {
        size_t last_size = size_;
        for (size_t i = 0; i < last_size; ++i) {
            pop_back();
        }
    }

    ~List() { Clear(); }

    List &operator=(const List &another_list) {
        if (AllocTraits::propagate_on_container_copy_assignment::value) {
            alloc_ = another_list.alloc_;
        }
        List copy(another_list);
        Swap(copy);
        return *this;
    }


    void Swap(List &another) {
        std::swap(size_, another.size_);
        if (another.fakeNode_.next == &another.fakeNode_) {
            another.fakeNode_.next = &fakeNode_;
        }

        if (fakeNode_.next == &fakeNode_) {
            fakeNode_.next = &another.fakeNode_;
        }


        if (another.fakeNode_.prev == &another.fakeNode_) {
            another.fakeNode_.prev = &fakeNode_;
        }

        if (fakeNode_.prev == &fakeNode_) {
            fakeNode_.prev = &another.fakeNode_;
        }

        std::swap(fakeNode_.next, another.fakeNode_.next);
        std::swap(fakeNode_.prev, another.fakeNode_.prev);

        fakeNode_.next->prev = &fakeNode_;
        fakeNode_.prev->next = &fakeNode_;
    }

    void push_back(const T &value) {
        Node *newNode = AllocTraits::allocate(alloc_, 1);
        AllocTraits::construct(alloc_, newNode, fakeNode_.prev, &fakeNode_, value);
        fakeNode_.prev->next = newNode;
        fakeNode_.prev = newNode;
        ++size_;
    }

    void pop_back() noexcept {
        if (size_ == 0 || fakeNode_.prev == &fakeNode_) {
            return;
        }
        Node *tmp = static_cast<Node *>(fakeNode_.prev);
        fakeNode_.prev = fakeNode_.prev->prev;
        fakeNode_.prev->next = &fakeNode_;
        AllocTraits::destroy(alloc_, tmp);
        AllocTraits::deallocate(alloc_, tmp, 1);
        --size_;
    }

    void push_front(const T &value) {
        Node *newNode = AllocTraits::allocate(alloc_, 1);
        AllocTraits::construct(alloc_, newNode, &fakeNode_, fakeNode_.next, value);
        fakeNode_.next = newNode;
        fakeNode_.next->next->prev = newNode;
        ++size_;
    }

    void pop_front() noexcept {
        if (size_ == 0 || fakeNode_.next == &fakeNode_) {
            return;
        }
        Node *tmp = static_cast<Node *>(fakeNode_.next);
        fakeNode_.next = tmp->next;
        fakeNode_.next->prev = &fakeNode_;
        AllocTraits::destroy(alloc_, tmp);
        --size_;
    }

    template<bool is_const>
    void insert(const BaseIter<is_const> &it, const T &object) {
        Node *newNode = AllocTraits::allocate(alloc_, 1);
        AllocTraits::construct(alloc_, newNode, it.iterator_->prev, it.iterator_,
                               object);
        it.iterator_->prev->next = newNode;
        it.iterator_->prev = newNode;
        ++size_;
    }


    template<bool is_const>
    void erase(const BaseIter<is_const> &it) {
        Node *delNode = static_cast<Node *>(it.iterator_);
        it.iterator_->prev->next = it.iterator_->next;
        it.iterator_->next->prev = it.iterator_->prev;
        AllocTraits::destroy(alloc_, delNode);
        AllocTraits::deallocate(alloc_, delNode, 1);
        --size_;
    }

    size_t size() const { return size_; }

    iterator begin() { return iterator(fakeNode_.next); }

    iterator end() { return iterator(&fakeNode_); }

    const_iterator begin() const { return const_iterator(fakeNode_.next); }

    const_iterator end() const { return const_iterator(&fakeNode_); }

    const_iterator cbegin() const { return begin(); }

    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return List::reverse_iterator(end()); }

    reverse_iterator rend() { return List::reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const {
        return List::const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const {
        return List::const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return List::const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const {
        return List::const_reverse_iterator(begin());
    }

    NodeAlloc get_allocator() const { return alloc_; }
};

template<size_t N>
class StackStorage {
public:
    char storage[N];
    size_t engaged = 0;

public:
    StackStorage() {};

    [[nodiscard]] constexpr char *allocate(size_t count, size_t size,
                                           size_t align) {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(&storage);
        if ((ptr + engaged) % align != 0) {
            engaged += align - ((ptr + engaged) % align);
        }
        char *ret = reinterpret_cast<char *>(ptr + engaged);
        engaged += count * size;
        if (engaged <= N) {
            return ret;
        }
        return nullptr;
    }
};

template<typename T, size_t N>
class StackAllocator {
public:
    StackStorage<N> *storage;

public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = int;

    StackAllocator() {}

    StackAllocator(StackStorage<N> &outer_storage) : storage(&outer_storage) {}

    template<typename U>
    StackAllocator(const StackAllocator<U, N> &other_allocator)
            : storage(other_allocator.storage) {}

    template<typename U>
    StackAllocator operator=(const StackAllocator<U, N> &other_allocator) {
        storage = other_allocator.storage;
        return *this;
    }

    constexpr T *allocate(size_t count) {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(storage);
        if ((ptr + storage->engaged) % alignof(T) != 0) {
            storage->engaged += alignof(T) - ((ptr + storage->engaged) % alignof(T));
        }
        T *ret = reinterpret_cast<T *>(ptr + storage->engaged);
        storage->engaged += count * sizeof(T);
        if (storage->engaged <= N) {
            return ret;
        }
        return nullptr;
    }


    void deallocate(T *, size_t) {}

    template<class U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

    template<typename T2, size_t N2>
    bool operator==(const StackAllocator<T2, N2> &another_allocater) {
        return storage == another_allocater.storage;
    }


    StackStorage<N> *getStorage() const { return storage; }
};
