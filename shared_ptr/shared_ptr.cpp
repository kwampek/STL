#include <vector>
#include <string>
#include <iterator>
#include <cassert>
#include <type_traits>

#include <iostream>

template<typename T>
class SharedPtr;

template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args &&... args);

class BaseControlBlock {
public:
    size_t shared_count = 0;
    size_t weak_count = 0;


    BaseControlBlock() = default;

    BaseControlBlock(const BaseControlBlock &another) : shared_count(another.shared_count),
                                                        weak_count(another.weak_count) {}

    BaseControlBlock(size_t shared_count, size_t weak_count) : shared_count(shared_count), weak_count(weak_count) {}

    virtual void destroy_field() = 0;

    virtual void destroy() = 0;

    virtual char *get_value() = 0;

    virtual ~BaseControlBlock() = default;
};

template<typename U, typename Alloc>
class ControlBlockMakeShared : public BaseControlBlock {
public:

    template<typename... Args>
    friend SharedPtr<U> allocateShared(Alloc alloc, Args &&... args);

    using AllocBlock = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockMakeShared<U, Alloc>>;
    using AllocTraits = std::allocator_traits<Alloc>;
    using AllocBlockTraits = std::allocator_traits<AllocBlock>;

private:
    alignas(U) char value[sizeof(U)];
    [[no_unique_address]] Alloc alloc;
public:

    void destroy_field() override {
        AllocTraits::destroy(alloc, reinterpret_cast<U *>(&value));
    }


    void destroy() override {
        AllocBlock allocator = static_cast<AllocBlock>(alloc);
        AllocBlockTraits::deallocate(allocator, this, 1);
    }

    char *get_value() override {
        return value;
    }

    template<typename ...Args>
    ControlBlockMakeShared(Alloc alloc, Args &&... args): alloc(alloc) {
        AllocTraits::construct(alloc, reinterpret_cast<U *>(&value), std::forward<Args>(args)...);
    }


    ~ControlBlockMakeShared() override = default;
};


template<typename T, typename Deleter = std::default_delete<T>, typename Alloc = std::allocator<T>>
class ControlBlockRegular : public BaseControlBlock {
public:

    template<typename... Args>
    friend SharedPtr<T> allocateShared(Alloc alloc, Args &&... args);

    using AllocBlock = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockRegular<T, Deleter, Alloc>>;
    using AllocBlockTraits = typename std::allocator_traits<AllocBlock>;

private:
    [[no_unique_address]] Deleter del;
    [[no_unique_address]] AllocBlock allocator;
public:
    T *ptr = nullptr;

    void destroy_field() override {
        del(ptr);
    }

    virtual void destroy() override {
        AllocBlockTraits::deallocate(allocator, this, 1);
    }

    char *get_value() override {
        return reinterpret_cast<char *>(ptr);
    }


    ControlBlockRegular(T *ptr, Deleter deleter = Deleter(), Alloc allocator = Alloc()) : del(deleter),
                                                                                          allocator(allocator),
                                                                                          ptr(ptr) {}

    ~ControlBlockRegular() = default;
};


template<typename T>
class EnableSharedFromThis;


template<typename T>
class WeakPtr;


template<typename T>
class SharedPtr {
private:
    T *ptr = nullptr;
    BaseControlBlock *pcb = nullptr;

    friend WeakPtr<T>;


    template<typename U>
    friend
    class SharedPtr;

    void safe_share_dec() {
        if (pcb) {
            --pcb->shared_count;
            if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
                if (ptr->wp.get() == get() && pcb->shared_count == 0 && pcb->weak_count == 1) {
                    destroy_field();
                    return;
                }
            }
            if (pcb->shared_count == 0) {
                destroy_field();
            }
            if (pcb->shared_count == 0 && pcb->weak_count == 0) {
                destroy();
            }
        }
    }

    void safe_share_inq() {
        if (pcb) {
            ++pcb->shared_count;
        }
    }

    void destroy_field() {
        pcb->destroy_field();
    }

    void destroy() {
        pcb->destroy();
        clear();
    }


    void clear() {
        ptr = nullptr;
        pcb = nullptr;
    }

public:


    T &operator*() const {
        return *get();
    }

    T *operator->() const {
        return get();
    }


    SharedPtr() : ptr(nullptr), pcb(nullptr) {}

    template<typename U>
    SharedPtr(U *ptr): ptr(static_cast<T *>(ptr)), pcb(new ControlBlockRegular<U>(ptr)) {
        safe_share_inq();
    }


    SharedPtr(T *ptr, BaseControlBlock *cb) : ptr(ptr), pcb(cb) {
        safe_share_inq();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            if (ptr) {
                ptr->wp = *this;
            }
        }
    }


    template<typename U>
    SharedPtr(const SharedPtr<U> &another) : ptr(static_cast<T *>(another.ptr)), pcb(static_cast<BaseControlBlock *>(
                                                                                             another.pcb)) {
        safe_share_inq();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            if (ptr) {
                ptr->wp = *this;
            }
        }
    }


    SharedPtr(SharedPtr<T> &&another) : ptr(another.ptr), pcb(
            another.pcb) {
        another.clear();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            if (ptr) {
                ptr->wp = *this;
            }
        }
    }


    template<typename U>
    SharedPtr(SharedPtr<U> &&another) : ptr(static_cast<T *>(another.ptr)), pcb(
            another.pcb) {
        another.clear();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            if (ptr) {
                ptr->wp = *this;
            }
        }
    }


    SharedPtr(const SharedPtr<T> &another) : ptr(another.ptr), pcb(
            another.pcb) {
        safe_share_inq();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            if (ptr) {
                ptr->wp = *this;
            }
        }
    }


    template<class U, class Deleter, class Alloc = std::allocator<T>>
    SharedPtr(U *ptrr, Deleter deleter = std::default_delete<T>(), Alloc alloc = std::allocator<T>()) : ptr(ptrr) {
        using AllocBlock = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockRegular<U, Deleter, Alloc>>;
        AllocBlock allocator = alloc;
        auto *p = std::allocator_traits<AllocBlock>::allocate(allocator, 1);
        new(p) ControlBlockRegular<U, Deleter, Alloc>(ptrr, deleter, alloc);
        pcb = p;
        safe_share_inq();
    }


    template<typename U>
    SharedPtr<T> &operator=(const SharedPtr<U> &other) {
        safe_share_dec();
        pcb = other.pcb;
        safe_share_inq();
        ptr = static_cast<T *>(other.ptr);
        return *this;
    }

    SharedPtr<T> &operator=(SharedPtr &&other) {
        safe_share_dec();
        pcb = other.pcb;
        ptr = other.ptr;
        other.clear();
        return *this;
    }


    template<typename U>
    SharedPtr<T> &operator=(SharedPtr<U> &&other) {
        safe_share_dec();
        pcb = other.pcb;
        ptr = static_cast<T *>(other.ptr);
        other.clear();
        return *this;
    }

    SharedPtr<T> &operator=(const SharedPtr &other) {
        safe_share_dec();
        pcb = other.pcb;
        safe_share_inq();
        ptr = static_cast<T *>(other.ptr);
        return *this;
    }

    T *get() const {
        return ptr;
    }


    void reset() noexcept {
        safe_share_dec();
        clear();
    }

    template<typename U>
    void reset(U *ptr) {
        reset();
        SharedPtr<U> new_ptr(ptr);
        swap(new_ptr);
    }


    void swap(SharedPtr &other) {
        std::swap(ptr, other.ptr);
        std::swap(pcb, other.pcb);
    }

    size_t use_count() const {
        if (pcb)
            return pcb->shared_count;
        return 0;
    }


    T *get_ptr() const {
        return ptr;
    }

    BaseControlBlock *get_pcb() const {
        return pcb;
    }

    ~SharedPtr() {
        safe_share_dec();
    }
};


template<typename T>
class WeakPtr {
private:
    T *ptr = nullptr;
    BaseControlBlock *pcb = nullptr;


    void clear() {
        ptr = nullptr;
        pcb = nullptr;
    }

public:
    WeakPtr() {};

    template<typename U>
    WeakPtr(const WeakPtr<U> &another): ptr(static_cast<T *>(another.get())),
                                        pcb(static_cast<BaseControlBlock *>(another.get_pcb())) {
        safe_weak_inq();
    }

    template<typename U>
    WeakPtr(const SharedPtr<U> &another): ptr(static_cast<T *>(another.get())),
                                          pcb(static_cast<BaseControlBlock *>(another.get_pcb())) {
        safe_weak_inq();
    }

    template<typename U>
    WeakPtr(WeakPtr<U> &&another): ptr(static_cast<T *>(another.get())),
                                   pcb(static_cast<BaseControlBlock *>(another.get_pcb())) {
        safe_weak_inq();
        another.clear();
    }


    template<typename U>
    WeakPtr(SharedPtr<U> &&another): ptr(static_cast<T *>(std::move(another.get()))),
                                     pcb(static_cast<BaseControlBlock *>(std::move(another.get_pcb()))) {
        safe_weak_inq();

    }


    template<typename U>
    WeakPtr &operator=(const WeakPtr<U> &another) {
        ptr = another.ptr;
        safe_weak_dec();
        pcb = another.control_block_;
        safe_weak_inq();
        return *this;
    }

    template<typename U>
    WeakPtr &operator=(const SharedPtr<U> &another) {
        ptr = another.ptr;
        safe_weak_dec();
        pcb = another.pcb;
        safe_weak_inq();
        return *this;
    }

    template<typename U>
    WeakPtr &operator=(WeakPtr<U> &&another) {
        ptr = another.ptr;
        safe_weak_dec();
        pcb = another.control_block_;
        safe_weak_inq();

        another.clear();
        return *this;
    }

    template<typename U>
    WeakPtr &operator=(SharedPtr<U> &&another) {
        ptr = another.ptr;
        safe_weak_dec();
        pcb = another.pcb;
        safe_weak_inq();

        another.safe_share_dec();
        another.clear();
        return *this;
    }


    bool expired() const {
        return !pcb || pcb->shared_count == 0;
    }

    SharedPtr<T> lock() const {
        if (!pcb) {
            throw std::runtime_error("what da dog doing");
        }
        SharedPtr ret(ptr, pcb);
        return ret;
    }


    void safe_weak_dec() {
        if (pcb) {
            --pcb->weak_count;
            if (pcb->shared_count == 0 && pcb->weak_count == 0) {
                pcb->destroy();
            }
        }
    }

    void safe_weak_inq() {
        if (pcb) {
            ++pcb->weak_count;
        }
    }

    size_t use_count() const {
        if (pcb) {
            return pcb->shared_count;
        }
        return 0;
    }

    T *get() const {
        return ptr;
    }

    BaseControlBlock *get_pcb() const {
        return pcb;
    }

    ~WeakPtr() {
        safe_weak_dec();
    }
};

template<typename T>
class EnableSharedFromThis {
private:

    WeakPtr<T> wp;

    template<typename U>
    friend
    class SharedPtr;

protected:
    EnableSharedFromThis() = default;

public:

    template<typename U, typename Alloc, typename... Args>
    friend SharedPtr<U> allocateShared(Alloc alloc, Args &&... args);

    SharedPtr<T> shared_from_this() const {
        return wp.lock();
    }
};


template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args &&... args) {
    using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
    using BlockaTraits = std::allocator_traits<BlockAlloc>;
    BlockAlloc ba = alloc;
    auto *ptr = BlockaTraits::allocate(ba, 1);
    //BlockaTraits::construct(ba, ptr, alloc, std::forward<Args>(args)...);
    new(ptr) ControlBlockMakeShared<T, Alloc>(alloc, std::forward<Args>(args)...);
    SharedPtr<T> ans(reinterpret_cast<T *>(ptr->get_value()), static_cast<BaseControlBlock *>(ptr));
    if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
        if (ptr) {
            reinterpret_cast<T *>(ptr->get_value())->wp = ans;
        }
    }
    return ans;
}


template<typename T, typename... Args>
SharedPtr<T> makeShared(Args &&... args) {
    return allocateShared<T, std::allocator<T>>(std::allocator<T>(), std::forward<Args>(args)...);
}
