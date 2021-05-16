#pragma once

#include "meta/ct_string.hpp"
#include "external/json.hpp"
#include "server/api_manager.hpp"


namespace scymnous
{
using json = nlohmann::json;


template<ct_string Name, class  T>
class path_param {
    T value;
public:
    constexpr path_param(): value{}
    {
    }

    path_param(T&& v): value(std::move(v)){
#ifdef TEST_DEBUG
        std::cout << "move ctor of path_param called with value: " << value  << std::endl;
#endif
    }

    T get(){
        return value;
    }
    static constexpr const char* name = Name.str();
    using type = T;
    static constexpr  ParameterType parameter_type = Path;



    static json describe() {
        json v;
        using dtype = std::decay_t<T>;
        v = traits<dtype>::describe();
        v["in"] = "path";
        v["required"] = true;
        v["name"] = name;
        return v;
    }
};


template<typename>
struct is_path_param : std::false_type {};

template<ct_string S, typename A>
struct is_path_param<path_param<S,A>> : std::true_type {};

template<class T>
constexpr bool  is_path_param_v = is_path_param<T>::value;





template<ct_string Name>
class segment_param {
    std::string value;
public:
    constexpr segment_param(): value{}
    {
    }

    segment_param(std::string&& v): value(std::move(v)){
#ifdef TEST_DEBUG
        std::cout << "move ctor of segment_param called with value: " << value  << std::endl;
#endif
    }

    std::string get(){
        return value;
    }
    static constexpr const char* name = Name.str();
    using type = std::string;

    //TODO: remove it is not used
    static constexpr  ParameterType parameter_type = Segment;

    static json describe() {
        return {};
    }
};


template<typename>
struct is_segment_param : std::false_type {};

template<ct_string S>
struct is_segment_param<segment_param<S>> : std::true_type {};

template<class T>
constexpr bool  is_segment_param_v = is_segment_param<T>::value;


template<typename T>
struct is_path_or_segment_param : std::disjunction<is_path_param<T>,is_segment_param<T>> {};

template<class T>
constexpr bool is_path_or_segment_param_v = is_path_or_segment_param<T>::value;




template<ct_string Name, class  T>
class header_param {
    T value;
public:

    constexpr header_param(): value{}{

    }

    header_param(T&& v): value(std::move(v)){
#ifdef TEST_DEBUG
        std::cout << "move ctor of header_param called with value: " << value  << std::endl;
#endif
    }

    T get() const{
        return value;
    }
    static constexpr const char* name = Name.str();
    using type = T;
    static constexpr  ParameterType parameter_type = Header;



    static json describe() {
        json v;
        using dtype = std::decay_t<T>;
        v = traits<dtype>::describe();
        v["in"] = "header";

        if (!is_optional_v<T>) {
            v["required"] = true;
        }

        v["name"] = name;
        return v;
    }

};


template<typename>
struct is_header_param : std::false_type {};

template<ct_string Name,typename T>
struct is_header_param<header_param<Name,T>> : std::true_type {};

template<typename T>
constexpr  bool is_header_param_v = is_header_param<T>::value;



template<ct_string Name, class  T>
class body_param {

    T value;

public:

    constexpr body_param(): value{}{

    }

    body_param(T&& v): value(std::move(v))
    {
    }

    const T& get() const{
        return value;
    }


    T& get() {
        return value;
    }

    static constexpr const char* name = Name.str();
    using type = T;
    static constexpr  ParameterType parameter_type = Body;



    static json describe() {
        using dtype = std::decay_t<T>;

        //check if type has name
        if  (dtype::name() != ""){
            doc_types::add_type<dtype>(dtype::name());
        }


        json v;

        v["in"] = "body";
        v["name"] = name;
        if (doc_types::get_type<dtype>()) {
            v["schema"]["$ref"] = std::string("#/definitions/") +
                                  doc_types::get_type<dtype>().value();
        } else {
            v["schema"] = traits<dtype>::describe();
        }

        //body is always required
        v["required"] = true;

        return v;

    }

};


template<typename>
struct is_body_param : std::false_type {};

template<ct_string Name,typename T>
struct is_body_param<body_param<Name,T>> : std::true_type {};

template<typename T>
constexpr  bool is_body_param_v = is_body_param<T>::value;



template<ct_string Name, class  T>
class query_param {

    T value;

public:

    constexpr query_param(): value{}{

    }

    query_param(T&& v): value(std::move(v)){
#ifdef TEST_DEBUG
        std::cout << "move ctor of query_param called with value: " << value  << std::endl;
#endif
    }

    T get(){ //make it move only?
        return value;
    }
    static constexpr const char* name = Name.str();
    using type = T;
    static constexpr  ParameterType parameter_type = Query;



    static json describe() {
        json v;
        using dtype = std::decay_t<T>;
        v = traits<dtype>::describe();
        v["in"] = "query";

        if (!is_optional_v<T>) {
            v["required"] = true;
        }

        v["name"] = name;
        return v;
    }

};


template<typename>
struct is_query_param : std::false_type {};

template<ct_string Name,typename T>
struct is_query_param<query_param<Name,T>> : std::true_type {};

template<typename T>
constexpr  bool is_query_param_v = is_query_param<T>::value;





} //namespace scymnous
