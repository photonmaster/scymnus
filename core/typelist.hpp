#pragma once

#include <type_traits>
#include <typeinfo>
#include <memory>
#include <string>
#include <cstdlib>


namespace scymnous {
namespace tl {

//identity type
template<class T> struct identity
{
    using type = T;
};


template<class... T> struct typelist {};
typedef struct empty_type{} empty;

//size
namespace detail {
template<class T> struct size;
template<class... T>
struct size<typelist<T...>>
{
    using type = std::integral_constant<std::size_t, sizeof...(T)>;
};
}

template<class T> using size = typename detail::size<T>::type;



template<class... T>
using length = std::integral_constant<std::size_t,sizeof...(T)>;


namespace detail {
template<class L, class... T>
struct push_front;

template<template <class...> class L,class...A,  class... T>
struct push_front<L<A...>,T...>{

    using type = L<T...,A...>;
};
}
template<class L, class... T>
using push_front = detail::push_front<L,T...>;


//transform
//It takes a metafunction and aplies it on a list producing a new list...
namespace detail {
template<template<class...> class F, class L> struct transform;
template<template<class...> class F, template<class...> class L, class... T>
struct transform<F, L<T...>>
{
    using type = L<F<T>...>;
};
}

template<template<class...> class F, class L>
using transform = typename detail::transform<F, L>::type;


//transform with a metafunction taking two parameters

namespace detail {

template<template <class...> class F, class L1, class L2>
struct transform_2;

template<template<class...> class F, template<class...> class L1,template<class...> class L2, class...A1,class...A2>
struct transform_2<F,L1<A1...>,L2<A2...>> {
    using type = L1<F<A1,A2>...>;
};

}

template<template <class...> class F, class L1, class L2>
using transform_2 = detail::transform_2<F,L1,L2>;



//merge, takes a series of lists and creates one typlist holding the types of all of them

namespace detail {

//REF: https://stackoverflow.com/questions/53394100/concatenating-tuples-as-types/53398815
template <typename T, typename ...>
struct merge {
    using type = T;
};


template <template <typename ...> class T,
         typename ... T1, typename ... T2, typename ... T3>
struct merge<T<T1...>, T<T2...>, T3...>
    : public merge<T<T1..., T2...>, T3...>
{ };

}

template <class... L>
using merge_t = typename detail::merge<L...>::type;

//append
namespace detail
{
template<class L, class A> struct append;

template<template<class...> class L,class...T, class A>
struct append<L<T...>,A>
{
    using type = L<T...,A>;
};
}

template<class L, class A>
using append = detail::append<L,A>;


namespace detail {

template<std::size_t N> using size_t = std::integral_constant<std::size_t, N>;

}


//index alternative implementation:
namespace detail{

template<class L, class V> struct index;

template<template<class...> class L, class V> struct index<L<>, V>
{
    using type = size_t<0>;
};

constexpr std::size_t cx_find_index( bool const * first, bool const * last )
{
    return first == last || *first? 0: 1 + cx_find_index( first + 1, last );
}

template<template<class...> class L, class... T, class V>
struct index<L<T...>, V>
{
    static constexpr bool _v[] = { std::is_same<T, V>::value... };

    using type = size_t< cx_find_index( _v, _v + sizeof...(T) ) >;
};

}

template<class L, class V> using index = typename detail::index<L, V>::type;



//prepend
namespace detail
{
template<class A,class L> struct prepend;

template<template<class...> class L,class...T, class A>
struct prepend<A,L<T...>>
{
    using type = L<A,T...>;
};
}

template<class A,class L>
using prepend = detail::prepend<A,L>;



//remove a type from a type list
namespace detail {

template<class T, class L, class R>
struct remove;

template<class T,template<class...> class L,class H, class...R,class Result>
struct remove<T,L<H, R...>,Result> {
    using type = typename remove<T,L<R...>,typename append<Result,H>::type>::type;
};

template<class T, template<class...> class L, class...R,class Result>
struct remove<T,L<T, R...>,Result> {

    using type = typename remove<T,L<R...>,Result>::type;
};

template<class T, template<class...> class L,class Result>
struct remove<T,L<T>,Result> {

    using type = Result;
};

template<class T, template<class...> class L,class H,class Result>
struct remove<T,L<H>,Result> {

    using type = typename append<Result,H>::type;
};

}

template<class T, class L>
using remove = typename detail::remove<T,L,typelist<>>::type;


//remove_if: remove all types that fullfill a unary predicate
namespace detail {

template<template<class> class P, class L, class R>
struct remove_if;

template<template<class> class P,template<class...> class L,class H, class...R,class Result>
struct remove_if<P,L<H, R...>,Result> {
    using type = typename remove_if
        <P,
         L<R...>,
         typename std::conditional
         <
             P<H>::value,
             Result,
             typename append<Result,H>::type
             >::type
         >::type;
};

//end condition
template<template<class> class P,template<class...> class L,class Result>
struct remove_if<P,L<>,Result> {
    using type = Result;
};
} //detail

template<template <class> class P, class L, class R = typelist<>>
using remove_if = typename detail::remove_if<P,L,R>::type;




//the oposite  operation to remove_if:  select_if

namespace detail {

template<template<class> class P, class L, class R>
struct select_if;

template<template<class> class P,template<class...> class L,class H, class...R,class Result>
struct select_if<P,L<H, R...>,Result> {
    using type = typename select_if
        <P,
         L<R...>,
         typename std::conditional
         <
             P<H>::value,
             typename append<Result,H>::type,
             Result
             >::type
         >::type;
};



//end condition
template<template<class> class P,template<class...> class L,class Result>
struct select_if<P,L<>,Result> {
    using type = Result;
};
} //detail

template<template <class> class P, class L, class R = typelist<>>
using select_if = typename detail::select_if<P,L,R>;


//TODO: fix this

template <typename T, typename TL>
struct contains_type;

template <typename T, template<class> class Container, typename... Us>
struct contains_type<T, Container<Us...>> : std::disjunction<std::is_same<T, Us>...> {};



//typelists_intersect

namespace detail {

template<class C1, class C2, class R>
struct intersect;


template<class C1,template<class...> class C2,class H, class...R,class Result>
struct intersect<C1,C2<H, R...>,Result> {
    using type = typename intersect
        <C1,
         C2<R...>,
         typename std::conditional
         <
           contains_type<H,C1>::value,
             typename append<Result,H>::type,
             Result
             >::type
         >::type;
};


template<class C1,template<class...> class C2,class Result>
struct intersect<C1,C2<>,Result> {
    using type = Result;
};

}

template<class C1, class C2, class R = typelist<>>
using intersect_t = typename detail::intersect<C1,C2,R>::type;



constexpr std::size_t npos = -1;

template <template <class T> class, typename, std::size_t pos = 0>
struct index_if;

template <template <class T> class Pred, typename T, std::size_t pos, typename... tail>
struct index_if<Pred, const std::tuple<T, tail...>, pos> :
    std::conditional<Pred<T>::value,
                     std::integral_constant<std::size_t, pos>,
                     index_if<Pred, const std::tuple<tail...>, pos+1>>::type {};

template <template <class T> class Pred>
struct index_if<Pred, const std::tuple<>> : std::integral_constant<std::size_t, npos> {};




//append_if

namespace detail {
template<template<class> class P, class L, class R>
struct append_if;

template<template<class> class P,template<class...> class L,class H, class...R,class Result>
struct append_if<P,L<H, R...>,Result> {
    using type = typename append_if
        <P,
         L<R...>,
         typename std::conditional
         <
             P<H>::value,
             typename append<Result,H>::type,
             Result
             >::type
         >::type;
};

//end condition
template<template<class> class P,template<class...> class L,class Result>
struct append_if<P,L<>,Result> {
    using type = Result;
};
} //detail

template<template <class> class P, class L, class R = typelist<>>
using append_if = typename detail::append_if<P,L,R>;



//reverse
//given a typelist<A1,A2,A3> the typlist<A3,A2,A1> is produced
namespace detail
{
template <class L>
struct reverse;

template<template <class...> class L, class... A, class A1>
struct reverse<L<A1,A...>>
{
    using type = typename append<typename reverse< L<A...>>::type,A1>::type;

};

template<template <class...> class L>
struct reverse<L<>>
{
    using type = L<>;
};
}

template <class L>
using reverse = typename detail::reverse<L>::type;


//normalise
namespace detail {

template<template<class> class P, class L1, class L2, class R>
struct normalise;

template<template<class> class P,template<class...> class L1,class H1, class...R1,template<class...> class L2,class H2, class...R2,class Result>
struct normalise<P,L1<H1, R1...>,L2<H2, R2...>,Result> {
    using type =
        typename std::conditional
        <
            P<H1>::value,
            typename normalise<P,L1<R1...>,L2<R2...>,typename append<Result,H2>::type>::type,
            typename normalise<P,L1<R1...>,L2<H2,R2...>,typename append<Result,empty_type>::type>::type
            >::type;
};

//end conditions
template<template<class> class P,template<class...> class L1,class L2,class Result>
struct normalise<P,L1<>,L2,Result> {
    using type = Result;
};
template<template<class> class P,template<class...> class L1,class H1, class...R1, template<class...> class  L2,class Result>
struct normalise<P,L1<H1, R1...>,L2<>,Result> {
    using type = typename normalise<P,L1<R1...>,L2<>,typename append<Result,empty_type>::type>::type;
};


} //detail

template<template <class> class P, class L1,class L2, class R = typelist<>>
using normalise = typename detail::normalise<P,L1,L2,R>;


template<bool... B>
struct and_;

template<bool B>
struct and_<B> : std::conditional_t<B,std::true_type,std::false_type>
{};


template<bool H, bool...T>
struct and_<H, T...> : std::conditional_t<H, and_<T...>,std::false_type>
{};


template <class T1,class T2>
struct is_same;


template <template <class...> class L1, class... A1, template <class...> class L2, class... A2>
struct is_same<L1<A1...>,L2<A2...>> : std::false_type
{};

template <template <class...> class L1, template <class...> class L2, class... A>
struct is_same<L1<A...>,L2<A...>> : std::true_type
{};



//change the name of the type
//e.g. rename<typelist<int, std::string>, std::tuple> is the type std::tuple<int, std::string>

namespace detail
{

template<class A, template<class...> class B> struct rename;
template<template<class...> class A, class... T, template<class...> class B>
struct rename<A<T...>, B>
{
    using type = B<T...>;
};
}

template<class A, template<class...> class B>
using rename_t = typename detail::rename<A, B>::type;


//appends an index
namespace detail
{
template<class S, std::size_t I>
struct append_index;


template<std::size_t...Is, std::size_t I>
struct append_index<std::index_sequence<Is...>,I>
{
    using type = std::index_sequence<Is...,I>;
};
}

template<class S, std::size_t I>
using append_index = detail::append_index<S,I>;



//add_index_if: given a list and a predicate, if the predicate is true for a type in the list
//the indexed position of the type is appended in the output index_sequence

namespace detail {
template<template<class> class P, class L, class R, std::size_t I>
struct append_index_if;

template<template<class> class P,template<class...> class L,class H, class...R,class Result, std::size_t I>
struct append_index_if<P,L<H, R...>,Result, I> {
    using type = typename append_index_if
        <P,
         L<R...>,
         typename std::conditional
         <
             P<H>::value,
             typename append_index<Result,I>::type,
             Result
             >::type,
         I + 1
         >::type;
};

//end condition
template<template<class> class P,template<class...> class L,class Result,std::size_t I>
struct append_index_if<P,L<>,Result,I> {
    using type = Result;
};
} //detail

template<template <class> class P, class L, class R = std::index_sequence<>,std::size_t I = 0>
using append_index_if = typename detail::append_index_if<P,L,R,I>;


//replace  a type with another one.
namespace detail {
template<class F,class T, class L>
struct replace;

//F: from
//T: To
template<class F,class T,template <class...> class L,class A1,class... A>
struct replace<F,T,L<A1,A...>>
{
    using type = typename prepend<typename replace<F,T,L<A...>>::type, A1>::type;

};


template<class F,class T,template <class...> class L,class... A>
struct replace<F,T,L<F,A...>>
{
    using type = typename prepend<typename replace<F,T,L<A...>>::type, T>::type;
};

} //detail



template<class From, class To, class L>
using replace = typename detail::replace<From,To,L>;


template<class, template <class> class...> struct any_of : std::false_type { };

template<class T, template <class> class P> struct any_of<T,P> : P<T> { };

template<class T, template <class> class P1, template <class> class... Pn>
struct any_of<T,P1, Pn...>
     : std::conditional_t<bool(P1<T>::value), P1<T>, any_of<T,Pn...>>  { };




} //namespace tl
} //namespace scymnous
