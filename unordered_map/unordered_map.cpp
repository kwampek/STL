#include <iostream>
#include <vector>
#include <type_traits>
#include <cmath>
#include <tuple>

template<typename T, typename Alloc = std::allocator<T>>
class List {
public:
    struct BaseNode {
        BaseNode* prev = nullptr;
        BaseNode* next = nullptr;

        BaseNode() {}

        BaseNode(const BaseNode& base_node)
                : prev(base_node.prev), next(base_node.next) {}

        BaseNode(BaseNode* prev, BaseNode* next) : prev(prev), next(next) {}
    };

    struct Node : BaseNode {
        T value;

        Node() : BaseNode() {}

        Node(BaseNode* prev, BaseNode* next, const T& value)
                : BaseNode(prev, next), value(value) {}

        Node(BaseNode* prev, BaseNode* next, T&& value) : BaseNode(prev, next), value(std::move(value)) {}


        Node(const BaseNode& node, const T& value) : BaseNode(node), value(value) {}

        Node(BaseNode* prev, BaseNode* next) : BaseNode(prev, next) {}

        Node(const Node& another)
                : BaseNode(another.prev, another.next), value(another.value) {
            ;
        }
    };

    template<bool is_const>
    struct BaseIter {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = int;
        using value_type = T;

        using reference = typename std::conditional<is_const, const T& , T& >::type;
        using pointer = typename std::conditional<is_const, const T* , T* >::type;
        using base_pointer =
                typename std::conditional<is_const, const BaseNode* , BaseNode* >::type;
        using node_pointer =
                typename std::conditional<is_const, const Node* , Node* >::type;

        BaseNode* iterator_;

        BaseIter<is_const>& operator++() {
            iterator_ = iterator_->next;
            return *this;
        }

        BaseIter<is_const>& operator--() {
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
        bool operator==(const BaseIter<another_is_const>& another) {
            return iterator_ == another.iterator_;
        }

        template<bool another_is_const>
        bool operator!=(const BaseIter<another_is_const>& another) {
            return iterator_ != another.iterator_;
        }

        reference operator*() const noexcept {
            return static_cast<node_pointer>(iterator_)->value;
        }

        pointer operator->() const noexcept {
            return& static_cast<node_pointer>(iterator_)->value;;
        }

        template<bool another_is_const>
        requires(std::is_same_v<BaseIter<is_const>, BaseIter<true>> &&
                 std::is_same_v<BaseIter<another_is_const>, BaseIter<false>>)
        BaseIter<is_const>& operator=(const BaseIter<another_is_const>& other) {
            iterator_ = other.iterator_;
            return *this;
        }

        template<bool another_is_const>
        requires(std::is_same_v<BaseIter<is_const>, BaseIter<true>> &&
                 std::is_same_v<BaseIter<another_is_const>, BaseIter<false>>)
        BaseIter(const BaseIter<another_is_const>& it) : iterator_(it.iterator_) {}

        BaseIter(base_pointer node) : iterator_(const_cast<BaseNode* >(node)) {}

        bool operator!=(const BaseIter<is_const>& another) const {
            return !(*this == another);
        }

        bool operator==(const BaseIter<is_const>& another) const {
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

    BaseNode fakeNode_;
    size_t size_;
    [[no_unique_address]] NodeAlloc alloc_;

    List() : fakeNode_(&fakeNode_,& fakeNode_), size_(0) {}

    List(size_t count, const T& elem, const Alloc& allocator = Alloc())
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

    List(const Alloc& allocator) : List() {
        alloc_ = AllocTraits::select_on_container_copy_construction(allocator);
    }


    List(size_t count, const Alloc& allocator = Alloc()) : List(allocator) {
        size_ = count;
        Node* tmp = static_cast<Node* >(&fakeNode_);
        size_t index = 0;
        try {
            Node* newNode = nullptr;
            for (; index < count; ++index) {
                newNode = AllocTraits::allocate(alloc_, 1);
                AllocTraits::construct(alloc_, newNode, static_cast<BaseNode* >(tmp),
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

    List(const List<T, Alloc>& another_list)
            : List(another_list.get_allocator()) {
        try {
            for (auto& node: another_list) {
                push_back(node);
            }
        } catch (...) {
            Clear();
            throw;
        }
    }

    List(List&& another_list) : List() {
        swap(another_list);

    }

    List& operator=(List&& another_list) {
        List Copy(std::move(another_list));
        swap(another_list);
        return *this;
    }


    void Clear() {
        size_t last_size = size_;
        for (size_t i = 0; i < last_size; ++i) {
            pop_back();
        }
    }

    ~List() { Clear(); }

    List& operator=(const List& another_list) {
        if (AllocTraits::propagate_on_container_copy_assignment::value) {
            alloc_ = another_list.alloc_;
        }
        Clear();

        size_t initialized = 0;
        try {
            for (auto it = another_list.begin(); it != another_list.end();) {
                push_back(static_cast<Node* >((it++).iterator_)->value);
                ++initialized;
            }
        } catch (...) {
            for (; initialized != 0; --initialized) {
                pop_back();
            }
        }
        return *this;
    }

    void swap(List& other) {
        std::swap(alloc_, other.alloc_);
        if (size_ == 0 && other.size_ == 0) {
            return;
        }
        if (other.size_ == 0) {
            other.fakeNode_.next = fakeNode_.next;
            other.fakeNode_.prev = fakeNode_.prev;
            fakeNode_.next = &fakeNode_;
            fakeNode_.prev = &fakeNode_;
        } else if (size_ == 0) {
            fakeNode_.next = other.fakeNode_.next;
            fakeNode_.prev = other.fakeNode_.prev;

            other.fakeNode_.next = &other.fakeNode_;
            other.fakeNode_.prev = &other.fakeNode_;
        } else {
            std::swap(fakeNode_.next, other.fakeNode_.next);
            std::swap(fakeNode_.prev, other.fakeNode_.prev);
        }
        std::swap(size_, other.size_);
        fakeNode_.next->prev = &fakeNode_;
        fakeNode_.prev->next = &fakeNode_;
        other.fakeNode_.next->prev = &other.fakeNode_;
        other.fakeNode_.prev->next = &other.fakeNode_;
    }


    template<typename ...Args>
    void push_back(Args&&... args) {
        Node* newNode = AllocTraits::allocate(alloc_, 1);
        AllocTraits::construct(alloc_, newNode, fakeNode_.prev,& fakeNode_, std::forward<Args>(args)...);
        fakeNode_.prev->next = newNode;
        fakeNode_.prev = newNode;
        ++size_;
    }


    void pop_back() noexcept {
        if (size_ == 0 || fakeNode_.prev ==& fakeNode_) {
            return;
        }
        Node* tmp = static_cast<Node* >(fakeNode_.prev);
        fakeNode_.prev = fakeNode_.prev->prev;
        fakeNode_.prev->next = &fakeNode_;
        AllocTraits::destroy(alloc_, tmp);
        AllocTraits::deallocate(alloc_, tmp, 1);
        --size_;
    }

    void push_front(const T& value) {
        Node* newNode = AllocTraits::allocate(alloc_, 1);
        AllocTraits::construct(alloc_, newNode,&fakeNode_, fakeNode_.next, value);
        fakeNode_.next = newNode;
        fakeNode_.next->next->prev = newNode;
        ++size_;
    }

    void pop_front() noexcept {
        if (size_ == 0 || fakeNode_.next == &fakeNode_) {
            return;
        }
        Node* tmp = static_cast<Node*>(fakeNode_.next);
        fakeNode_.next = tmp->next;
        fakeNode_.next->prev = &fakeNode_;
        AllocTraits::destroy(alloc_, tmp);
        --size_;
    }

    template<bool is_const>
    void insert(const BaseIter<is_const>& it, const T& object) {
        Node* newNode = AllocTraits::allocate(alloc_, 1);
        AllocTraits::construct(alloc_, newNode, it.iterator_->prev, it.iterator_,
                               object);
        it.iterator_->prev->next = newNode;
        it.iterator_->prev = newNode;
        ++size_;
    }

    void connect_node(BaseNode* iterator, BaseNode* new_node) {
        auto prev = iterator->prev;
        new_node->prev = prev;
        new_node->next = iterator;
        new_node->next->prev = new_node;
        new_node->prev->next = new_node;
        ++size_;
    }

    template<bool is_const>
    void erase(const BaseIter<is_const>& it) {
        Node* delNode = static_cast<Node* >(it.iterator_);
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


template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = std::allocator<Key>>
class UnorderedMap {
public:
    using NodeType = std::pair<const Key, Value>;
    using NodeTypePointer = NodeType*;
    using NodeTypeReference = NodeType&;

    struct HashedNode {
        NodeType node_value;
        size_t cached_hash = 0;

        HashedNode() {}

        HashedNode(const Key& key, const Value& value) : node_value(key, value) {}

        HashedNode(const NodeType& value) : node_value(value) {}

        HashedNode(NodeType&& value) : node_value(std::move(value)) {}

    };


    using list = List<HashedNode, Alloc>;


    using base_node = list::BaseNode;
    using base_node_pointer = base_node*;
    using node = list::Node;
    using node_pointer = node*;


    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<node>;
    using AllocTraits = std::allocator_traits<NodeAlloc>;


    template<bool is_const>
    struct BaseIter {
        using iterator_category = std::forward_iterator_tag;
        using value_type = NodeType;
        using difference_type = int;

        using reference = typename std::conditional<is_const, const NodeType& , NodeType& >::type;
        using pointer = typename std::conditional<is_const, const NodeType* , NodeType* >::type;
        using cond_base_pointer =
                typename std::conditional<is_const, const base_node_pointer, base_node_pointer>::type;
        using cond_node_pointer =
                typename std::conditional<is_const, const node_pointer, node_pointer>::type;

        base_node_pointer iterator_;

        BaseIter<is_const>& operator++() {
            iterator_ = iterator_->next;
            return *this;
        }

        BaseIter<is_const> operator++(int) {
            BaseIter<is_const> old_iterator = *this;
            iterator_ = iterator_->next;
            return old_iterator;
        }

        template<bool another_is_const>
        bool operator==(const BaseIter<another_is_const>& another) {
            return iterator_ == another.iterator_;
        }

        template<bool another_is_const>
        bool operator!=(const BaseIter<another_is_const>& another) {
            return iterator_ != another.iterator_;
        }

        reference operator*() const noexcept {
            return static_cast<cond_node_pointer>(iterator_)->value.node_value;
        }

        pointer operator->() const noexcept {
            return& static_cast<cond_node_pointer>(iterator_)->value.node_value;
        }

        template<bool another_is_const>
        requires(std::is_same_v<BaseIter<is_const>, BaseIter<true>> &&
                 std::is_same_v<BaseIter<another_is_const>, BaseIter<false>>)
        BaseIter<is_const>& operator=(const BaseIter<another_is_const>& other) {
            iterator_ = other.iterator_;
            return *this;
        }

        template<bool another_is_const>
        requires(std::is_same_v<BaseIter<is_const>, BaseIter<true>> &&
                 std::is_same_v<BaseIter<another_is_const>, BaseIter<false>>)
        BaseIter(const BaseIter<another_is_const>& it) : iterator_(it.iterator_) {}

        BaseIter(base_node_pointer node) : iterator_(node) {}


        bool operator!=(const BaseIter<is_const>& another) const {
            return !(*this == another);
        }

        bool operator==(const BaseIter<is_const>& another) const {
            return iterator_ == another.iterator_;
        }
    };


    using iterator = BaseIter<false>;
    using const_iterator = BaseIter<true>;


    using isplaced = std::pair<iterator, bool>;
private:


    std::vector<base_node_pointer,
            typename std::allocator_traits<Alloc>::template rebind_alloc<base_node_pointer>> buckets_;
    list nodes_;


    double max_load_factor_ = 0.95;


    [[no_unique_address]] NodeAlloc allocator_;
    [[no_unique_address]] Hash hasher_;
    [[no_unique_address]] Equal equal_;

public:
    void status() {
        nodes_.status();
    }

    UnorderedMap() = default;

    UnorderedMap(const UnorderedMap& unorderedMap) {
        try {
            for (auto it = unorderedMap.begin(); it != unorderedMap.end(); ++it) {
                insert(*it);
            }
        } catch (...) {
            Clear();
        }
    }

    UnorderedMap(UnorderedMap&& another) : buckets_(std::move(another.buckets_)), nodes_(std::move(another.nodes_)),
                                           max_load_factor_(another.max_load_factor_),
                                           allocator_(std::move(another.allocator_)),
                                           hasher_(std::move(another.hasher_)), equal_(std::move(another.equal_)) {
    }


    UnorderedMap& operator=(const UnorderedMap& another) {
        if (&another != this) {
            UnorderedMap copy(another);
            swap(copy);
        }
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& another) noexcept {
        if (&another != this) {
            UnorderedMap copy(std::move(another));
            swap(copy);
        }
        return *this;
    }

    template<typename ...Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        node_pointer newNode = AllocTraits::allocate(allocator_, 1);
        try {
            AllocTraits::construct(allocator_,& (newNode->value.node_value), std::forward<Args>(args)...);
        } catch (...) {
            AllocTraits::deallocate(allocator_, newNode, 1);
            return except_value();
        }

        try {

            check_load();

            newNode->value.cached_hash = hasher_(newNode->value.node_value.first);
            size_t hash = newNode->value.cached_hash % buckets_.size();
            auto found = find(newNode->value.node_value.first);
            if (found != end()) {
                AllocTraits::destroy(allocator_, newNode);
                AllocTraits::deallocate(allocator_, newNode, 1);
                return {found, false};
            }
            nodes_.connect_node(buckets_[hash] ? buckets_[hash] : begin().iterator_, newNode);
            buckets_[hash] = newNode;

        } catch (...) {
            AllocTraits::destroy(allocator_, newNode);
            AllocTraits::deallocate(allocator_, newNode, 1);
            return except_value();
        }
        return {newNode, true};
    }


    void Clear() {
        node_pointer tmp;
        for (auto it = begin(); it != end(); ++it, nodes_.pop_front()) {
            tmp = static_cast<node_pointer>(it.iterator_);
            buckets_[tmp->value.cached_hash] = nullptr;
        }
    }

    size_t size() const {
        return nodes_.size();
    }


    void swap(UnorderedMap& other) {
        std::swap(allocator_, other.allocator_);
        std::swap(hasher_, other.hasher_);
        std::swap(equal_, other.equal_);
        std::swap(buckets_, other.buckets_);
        nodes_.swap(other.nodes_);
    }


    ~UnorderedMap() = default;

    iterator begin() {
        return nodes_.begin().iterator_;
    }

    iterator end() {
        return nodes_.end().iterator_;
    }


    const_iterator begin() const {
        return nodes_.cbegin().iterator_;
    }

    const_iterator end() const {
        return nodes_.cend().iterator_;
    }


    const_iterator cbegin() const {
        return const_iterator(nodes_.begin().iterator_);
    }

    const_iterator cend() const {
        return nodes_.cend().iterator_;
    }


    Value& operator[](Key&& key) {
        iterator it = find(key);
        if (it == end()) {
            it = insert(key, Value()).first;
        }
        return static_cast<node_pointer>(it.iterator_)->value.node_value.second;
    }

    Value& operator[](const Key& key) {
        iterator it = find(key);
        if (it == end()) {
            it = insert(key, Value()).first;
        }
        return static_cast<node_pointer>(it.iterator_)->value.node_value.second;
    }


    Value& at(const Key& key) {
        auto iterator = find(key);
        if (iterator == end()) {
            throw std::out_of_range("out of range");
        }
        return static_cast<node_pointer>(iterator.iterator_)->value.node_value.second;
    }


    iterator find(const Key& key) {
        size_t hash;
        if (buckets_.size() == 0 || !buckets_[hash = (hasher_(key) % buckets_.size())]) {
            return end();
        }
        node_pointer tmp;
        try {
            for (auto it = buckets_[hash];
                 it != end().iterator_ &&
                 (tmp = static_cast<node_pointer>(it))->value.cached_hash % buckets_.size() == hash; it = it->next) {
                tmp = static_cast<node_pointer>(it);
                if (equal_(key, tmp->value.node_value.first)) {
                    return it;
                }
            }
        } catch (...) {}

        return end();
    }

    void rehash(size_t count) {
        buckets_.assign(count, nullptr);
        list copy(std::move(nodes_));
        size_t hash;
        for (auto iterator = copy.begin(); iterator != copy.end();) {
            auto next = iterator.iterator_->next;
            hash = static_cast<node_pointer>(iterator.iterator_)->value.cached_hash % buckets_.size();
            if (buckets_[hash] == nullptr) {
                nodes_.connect_node(nodes_.fakeNode_.next, iterator.iterator_);
                buckets_[hash] = iterator.iterator_;
            } else {
                nodes_.connect_node(buckets_[hash]->next, iterator.iterator_);
            }
            iterator = next;
        }
        copy.size_ = 0;
    }


    isplaced insert(const NodeType& elem) {
        return emplace(std::move(elem));
    }

    template<typename... Args>
    isplaced insert(Args&&... args) {
        return emplace(std::forward<Args>(args)...);
    }

    template<class InputIt>
    void insert(InputIt begin, InputIt end) {
        std::vector<iterator> to_delete;
        try {
            for (auto it = begin; it != end; ++it) {
                auto result = insert(*it);
                if (result.second) {
                    to_delete.push_back(result.first);
                }
            }
        } catch (...) {
            for (auto iter: to_delete) {
                erase(iter);
            }
        }

    }

    template<class InputIt>
    void erase(const InputIt& iter) {
        size_t iter_hash = static_cast<node_pointer>(iter.iterator_)->value.cached_hash % buckets_.size();
        if (buckets_[iter_hash] == iter.iterator_) {
            if (static_cast<node_pointer>(iter.iterator_->next)->value.cached_hash % buckets_.size() == iter_hash) {
                buckets_[iter_hash] = iter.iterator_->next;
            } else {
                buckets_[iter_hash] = nullptr;
            }
        }
        nodes_.erase(typename list::template BaseIter<false>(iter.iterator_));
    }

    template<class InputIt>
    void erase(const InputIt& begin, const InputIt& end) {
        for (auto it = begin; it != end;) {
            erase(it++);
        }
    }


    float max_load_factor() const {
        return max_load_factor_;
    }

    float load_factor() const {
        if (buckets_.size() == 0) {
            return 0;
        }
        return static_cast<float>(size()) / static_cast<float>(buckets_.size());
    }


    void check_load() {
        if (buckets_.size() == 0 || load_factor() >= max_load_factor_) {
            rehash(2 * buckets_.size() + 1);
        }
    }

    void reserve(size_t count) {
        if (size() >= std::ceil(count / max_load_factor())) {
            return;
        }
        rehash(std::ceil(count / max_load_factor()));
    }

    isplaced except_value() {
        return {nodes_.end().iterator_, false};
    }
};
