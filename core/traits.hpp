#pragma once
#include <utility>
#include <optional>
#include <vector>

namespace scymnous {

template<class T>
struct is_optional : std::false_type{};

template<class T>
struct is_optional<std::optional<T>> : std::true_type{};

template<class T>
constexpr bool is_optional_v = is_optional<T>::value;


template<typename>
struct is_vector : std::false_type {};

template<typename T, typename A>
struct is_vector<std::vector<T,A>> : std::true_type {};


template<class T>
constexpr  bool is_vector_v = is_vector<T>::value;

//namespace details {
//template <template <typename...> class base,typename derived>
//using is_base_of_model = typename std::is_base_of<base,derived>::type;
//}

//template <typename derived>
//using is_base_of_model = typename details::is_base_of_model<model,derived>::type;



} //namespace scymnous
