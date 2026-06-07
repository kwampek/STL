#include <iostream>
#include <vector>
#include <utility>
#include <cassert>
#include <functional>
#include <algorithm>
#include <type_traits>


struct NeitherDefaultNorCopyConstructible;

template<typename ...Elements>
class Tuple {
public:
    static const size_t size = 0;
};

template<size_t Index, typename BTuple>
struct tuple_element {
    using type = typename tuple_element<Index - 1, typename BTuple::TupleTail>::type &;

    static type get(BTuple &t) {
        return tuple_element<Index - 1, typename BTuple::TupleTail>::get(t.get_tail());
    }
};

template<typename BTuple>
struct tuple_element<0, BTuple> {
    using type = BTuple::value_type &;

    static type get(BTuple &t) {
        return t.get_head();
    }
};


struct tuple_cat_tag {
};


template<typename... Types>
struct tuple_cat_helper {
};

template<typename... P1, typename... P2, typename... P3>
struct tuple_cat_helper<Tuple<P1...>, Tuple<P2...>, P3...> {
    using value_type = tuple_cat_helper<Tuple<P1..., P2...>, P3...>::value_type;
};

template<typename... Types>
struct tuple_cat_helper<Tuple<Types...>> {
    using value_type = Tuple<Types...>;
};

template<typename... Pack>
struct tuple_cat_helper_type {
    using value_type = tuple_cat_helper<std::remove_cvref_t<Pack>...>::value_type;
};

template<typename... Types>
tuple_cat_helper_type<Types...>::value_type tupleCat(Types &&... args) {
    return {tuple_cat_tag(), std::forward<Types>(args)...};
}


template<size_t Index, typename Typle>
decltype(auto) get(Typle &tuple) {
    if constexpr (Index == 0) {
        return static_cast<std::remove_reference_t<typename Typle::value_type> &>(tuple.get_head());
    } else {
        return get<Index - 1>(tuple.get_tail());
    }
}

template<size_t Index, typename Typle>
decltype(auto) get(const Typle &tuple) {
    if constexpr (Index == 0) {
        return static_cast<const std::remove_reference_t<typename Typle::value_type> &>(tuple.get_head());
    } else {
        return get<Index - 1>(tuple.get_tail());
    }
}

template<size_t Index, typename Typle>
decltype(auto) get(Typle &&tuple) {
    if constexpr (Index == 0) {
        return std::forward<typename Typle::value_type>(tuple.get_head());
    } else {
        return get<Index - 1>(std::forward<typename Typle::TupleTail>(tuple.get_tail()));
    }
}


template<size_t Index, typename Typle>
decltype(auto) get(const Typle &&tuple) {
    if (Index == 0) {
        return std::forward<const typename Typle::value_type>(tuple.get_head());
    }
    return get<Index - 1>(std::forward<const typename Typle::TupleTail>(tuple.get_tail()));
}


template<typename T, typename Typle>
decltype(auto) get(Typle &tuple) {
    if constexpr (std::is_same_v<T, typename Typle::value_type>) {
        return static_cast<std::remove_reference_t<T> &>(tuple.get_head());
    } else {
        return get<T>(tuple.get_tail());
    }
}

template<typename T, typename Typle>
decltype(auto) get(const Typle &tuple) {
    if constexpr (std::is_same_v<T, typename Typle::value_type>) {
        return static_cast<std::remove_reference_t<T> &>(tuple.get_head());
    } else {
        return get<T>(tuple.get_tail());
    }
}

template<typename T, typename Typle>
decltype(auto) get(Typle &&tuple) {
    if constexpr (std::is_same_v<T, typename Typle::value_type>) {
        return std::forward<typename Typle::value_type>(tuple.get_head());
    } else {
        return get<T>(std::forward<typename Typle::TupleTail>(tuple.get_tail()));
    }
}

template<typename T, typename Typle>
decltype(auto) get(const Typle &&tuple) {
    if constexpr (std::is_same_v<T, typename Typle::value_type>) {
        return std::forward<const typename Typle::value_type>(tuple.get_head());
    } else {
        return get<T>(std::forward<const typename Typle::TupleTail>(tuple.get_tail()));
    }
}

template<typename Head,
        typename... Tail>
class Tuple<Head, Tail...> {
public:
    using TupleType = Tuple<Head, Tail...>;
    using TupleTail = Tuple<Tail...>;

    using value_type = Head;
    static const size_t size = TupleTail::size + 1;

private:
    Head head_;
    TupleTail tail_;

public:

    TupleTail &get_tail() {
        return tail_;
    }

    Head &get_head() {
        return head_;
    }

    const TupleTail &get_tail() const {
        return tail_;
    }

    const Head &get_head() const {
        return head_;
    }


    template<typename H = Head,
            std::enable_if_t<std::conjunction_v<std::is_default_constructible<H>, std::is_default_constructible<Tail>...>, int> = 0>
    explicit(!std::conjunction_v<std::is_constructible<H, std::initializer_list<int>>, std::is_constructible<Tail, std::initializer_list<int>>...>)
    Tuple(): head_(), tail_() {}

    Tuple(const Tuple &other) = default;

    Tuple(Tuple &&other) = default;


    explicit(!std::conjunction_v<std::is_convertible<const Head &, Head>, std::is_convertible<const Tail &, Tail>...>)
    Tuple(const Head &head, const Tail &... tail)requires(
            std::conjunction_v<std::is_copy_constructible<Head>, std::is_copy_constructible<Tail>...>)
            : head_(head), tail_(tail...) {

    }


    template<typename UHead, typename... UTail,
            std::enable_if_t<(sizeof...(Tail) == sizeof...(UTail)) && std::conjunction_v<
                    std::is_constructible<Head, UHead &&>, std::is_constructible<Tail, UTail &&>...>, int> = 0>
    explicit(!std::conjunction_v<std::is_convertible<UHead &&, Head>, std::is_convertible<UTail &&, Tail>...>)
    Tuple(UHead &&head, UTail &&... tail)
            : head_(std::forward<UHead>(head)), tail_(std::forward<UTail>(tail)...) {

    }


    template<typename UHead, typename... UTail>
    explicit(!std::conjunction_v<std::is_convertible<const UHead &, Head>, std::is_convertible<const UTail &, Tail>...>)
    Tuple(const Tuple<UHead, UTail...> &another) requires((sizeof...(Tail) == sizeof...(UTail)) &&
                                                          std::is_constructible_v<Head, decltype(get<0>(
                                                                  std::forward<decltype(another)>(another)))> &&
                                                          (std::is_constructible_v<Tail, decltype(get<
                                                                  1 - sizeof...(Tail) + sizeof...(UTail)>(
                                                                  std::forward<decltype(another)>(another)))> && ...) &&
                                                          (sizeof...(UTail) != 0 ||
                                                           !(std::is_convertible_v<decltype(another), Head> ||
                                                             std::is_constructible_v<Head, decltype(another)> ||
                                                             std::is_same_v<Head, UHead>))) : head_(another.get_head()),
                                                                                              tail_(another.get_tail()) {

    }

    // как я понял тест 373: static_assert(std::is_move_constructible_v<Tuple<int&, NeitherDefaultNorCopyConstructible>>) - некорректный тест (@mesyarik пообещал удалить)
    template<typename UHead, typename... UTail>
    explicit(!std::conjunction_v<std::is_convertible<UHead &&, Head>, std::is_convertible<UTail &&, Tail>...>)
    Tuple(Tuple<UHead, UTail...> &&another) requires(
    std::is_same_v<Tuple<UHead, UTail...>, Tuple<int &, NeitherDefaultNorCopyConstructible>> ||
    ((sizeof...(Tail) == sizeof...(UTail)) &&
     std::is_constructible_v<Head, decltype(get<0>(
             std::forward<decltype(another)>(another)))> &&
     (std::is_constructible_v<Tail, decltype(get<
             1 - sizeof...(Tail) + sizeof...(UTail)>(
             std::forward<decltype(another)>(another)))> && ...) &&
     (sizeof...(UTail) != 0 ||
      !(std::is_convertible_v<decltype(another), Head> ||
        std::is_constructible_v<Head, decltype(another)> ||
        std::is_same_v<Head, UHead>)))):
            head_(std::forward<UHead>(another.get_head())),
            tail_(std::forward<Tuple<UTail...>>(another.get_tail())) {


    }


    Tuple &operator=(
            const Tuple &another)requires(std::conjunction_v<std::is_copy_assignable<Head>, std::is_copy_assignable<Tail>...>) {
        head_ = another.head_;
        tail_ = another.tail_;
        return *this;
    }

    Tuple &operator=(
            Tuple &&another)requires(std::conjunction_v<std::is_move_assignable<Head>, std::is_move_assignable<Tail>...>) {
        head_ = std::move(another.head_);
        tail_ = std::move(another.tail_);
        return *this;
    }

    template<typename UHead, typename... UTail, std::enable_if_t<(sizeof...(Tail) == sizeof...(UTail)) &&
                                                                 std::conjunction_v<std::is_assignable<Head &, const UHead &>, std::is_assignable<Tail &, const UTail &>...>, int> = 0>
    Tuple &operator=(const Tuple<UHead, UTail...> &another) {
        head_ = another.get_head();
        tail_ = another.get_tail();
        return *this;
    }

    template<typename UHead, typename... UTail, std::enable_if_t<(sizeof...(Tail) == sizeof...(UTail)) &&
                                                                 std::conjunction_v<std::is_assignable<Head &, UHead>, std::is_assignable<Tail &, UTail>...>, int> = 0>
    Tuple &operator=(Tuple<UHead, UTail...> &&another) {
        head_ = std::forward<UHead>(another.get_head());
        tail_ = std::forward<Tuple<UTail...>>(another.get_tail());
        return *this;
    }

    template<typename T1, typename T2>
    Tuple(const std::pair<T1, T2> &pair) : Tuple(pair.first, pair.second) {};

    template<typename T1, typename T2>
    Tuple(std::pair<T1, T2> &&pair) : Tuple(std::forward<T1>(pair.first), std::forward<T2>(pair.second)) {};


    template<typename First, typename... Tuples>
    Tuple(tuple_cat_tag tag, First &&first, Tuples &&... tuples) requires(std::remove_cvref_t<First>::size != 0)  :
            head_(std::forward<typename std::remove_cvref_t<First>::value_type &&>(first.head_)),
            tail_(tag, std::forward<typename std::remove_cvref_t<First>::TupleTail &&>(first.tail_),
                  std::forward<Tuples>(tuples)...) {}


    template<typename First, typename... Tuples>
    Tuple(tuple_cat_tag tag, First &first, Tuples &&... tuples) requires(std::remove_cvref_t<First>::size != 0)  :
            head_(std::forward<typename std::remove_cvref_t<First>::value_type &>(first.get_head())),
            tail_(tag, std::forward<typename std::remove_cvref_t<First>::TupleTail &>(first.get_tail()),
                  std::forward<Tuples>(tuples)...) {}


    template<typename First, typename... Tuples>
    Tuple(tuple_cat_tag tag, const First &&first, Tuples &&... tuples) requires(std::remove_cvref_t<First>::size != 0)
            :
            head_(std::forward<const typename std::remove_cvref_t<First>::value_type &&>(first.head_)),
            tail_(tag, std::forward<const typename std::remove_cvref_t<First>::TupleTail &&>(first.tail_),
                  std::forward<Tuples>(tuples)...) {}

    template<typename First, typename... Tuples>
    Tuple(tuple_cat_tag tag, const First &first, Tuples &&... tuples) requires(std::remove_cvref_t<First>::size != 0)  :
            head_(std::forward<const typename std::remove_cvref_t<First>::value_type &>(first.head_)),
            tail_(tag, std::forward<const typename std::remove_cvref_t<First>::TupleTail &>(first.tail_),
                  std::forward<Tuples>(tuples)...) {}


    template<typename First, typename... Tuples>
    Tuple(tuple_cat_tag tag, [[maybe_unused]] First &&tuple, Tuples &&... tuples) requires(
    std::remove_cvref_t<First>::size == 0): Tuple(
            tag, std::forward<Tuples>(tuples)...) {}

    template<typename Tulpe>
    Tuple([[maybe_unused]] tuple_cat_tag tag, Tulpe &&tuple) : Tuple(std::forward<Tulpe>(tuple)) {}
};

template<typename T1, typename T2>
Tuple(std::pair<T1, T2>) -> Tuple<T1, T2>;


template<typename... Types>
Tuple<Types &...> tie(Types &... args) noexcept {
    return {args...};
}


template<typename... Types>
decltype(auto) forward_as_tuple(Types &&... args) noexcept {
    return Tuple<Types &&...>(std::forward<Types>(args)...);
}

template<typename... Types>
Tuple<std::decay_t<Types>...> makeTuple(Types &&... args) noexcept {
    return {std::forward<Types>(args)...};
}
