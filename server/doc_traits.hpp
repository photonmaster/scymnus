#pragma once

#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <vector>
#include <boost/lexical_cast.hpp>


#include "external/json.hpp"
#include "doc_common.hpp"
#include "core/named_tuple.hpp"
#include "core/traits.hpp"
#include "core/enumeration.hpp"

#include "core/response.hpp"


#include "properties/defaulted.hpp"




namespace scymnus {

template<class T, class U = void>
struct traits;

//common types
using json = nlohmann::json;





class  doc_types
{

public:
    template<class T>
    static void add_type(const std::string& name);

    template<class T>
    static std::optional<std::string> get_type();

    static json& get_definitions() {
        static json definitions_;
        return  definitions_;
    }

private:
    using models_cont = std::unordered_map<std::type_index, std::string> ;
    static models_cont& types() {
        static models_cont models;
        return  models;
    }

};


template<class T>
void doc_types::add_type(const std::string& name){
    if (types().count(std::type_index(typeid(T)))) return;
    types()[std::type_index(typeid(T))] = name;

    get_definitions()[name] = traits<T>::describe();
}

template<class T>
std::optional<std::string> doc_types::get_type(){
    if (types().count(std::type_index(typeid(T))))
        return types()[std::type_index(typeid(T))];
    else
        return{};
}








#define SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(type, type_name, format) \
    template<> \
    struct traits<type> { \
    static json describe() \
    { \
        return {{"type",#type_name},{"format",#format}}; \
    } \
};



SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(int8_t, integer,int8)
SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(int16_t, integer,int16)
SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(int32_t, integer,int32)
SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(int64_t, integer,int64)

SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(uint8_t, integer,uint8)
SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(uint16_t, integer,uint16)
SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(uint32_t, integer,uint32)
SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(uint64_t, integer,uint64)

SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(float, number,float)
SCYMNUS_SPECIALIZE_COMMON_TYPES_TRAIT(double, integer,double)

//take care of date anddatetime types
//take care of decimal type


template<>
struct traits<bool> {
    static json describe()
    {
        return {{"type","boolean"}};
    }
};


template<>
struct traits<std::string> {
    static json describe()
    {
        return {{"type","string"}};
    }
};



template<>
struct traits<json> {
    static json describe()
    {
        return {{"type","string"},{"format","json"}};
    }
};



template<class T>
struct traits<std::optional<T>> {
    static json describe()
    {
        json v;
        auto ref = doc_types::get_type<T>();
        if (ref)
            v["$ref"]  = "#/definitions/" + ref.value();
        else
            v  = traits<T>::describe();
        return v;
    }
};


template<class T>
struct traits<std::vector<T>>
{
    static json describe(){
        json v;
        v["type"] = "array";
        if (doc_types::get_type<T>())
            v["items"]["$ref"] = std::string("#/definitions/") + doc_types::get_type<T>().value();
        else
            v["items"] = traits<T>::describe();
        return v;
    }
};



template<typename... T>
struct traits<model<T...>> {
    static json describe()
    {
        json v;
        using type = model<T...>;

        v["type"] = "object";

        v["description"] = type::description();
        for_each(type{},([&v] (auto&& f, auto&& value){

                     using properties = std::remove_cvref_t<decltype(f.properties)>;
                     using type = std::remove_cvref_t<decltype(value)>;

                     //TODO: handle recursive
                     //TODO: handle hidden


                     if constexpr(scymnus::has_type<typename constraints::hidden<bool>, properties>::value){

                         //TODO: check why the if when moved to the if constexpr above eveything breaks
                         if ( std::get<constraints::hidden<bool>>(f.properties).value())
                             return;
                     }

                     if(!(is_optional<std::decay_t<decltype(value)>>::value))
                         v["required"].push_back(f.name);

                     auto ref = doc_types::get_type<type>();
                     if (ref)
                         v["properties"][f.name]["$ref"] = "#/definitions/" + ref.value();
                     else{
                         json child = traits<type>::describe();
                         v["properties"][f.name] = child;
                         if(!child.count("$ref")){
                             v["properties"][f.name]["description"] = f.name;

                             if constexpr (is_optional_v<type>){
                                 using nested_type = type::value_type;

                                 //TODO: check min vs max
                                 if constexpr(scymnus::has_type<constraints::min<nested_type>, properties>::value)
                                     v["properties"][f.name]["minimum"] = std::get<constraints::min<nested_type>>(f.properties).value();

                                 if constexpr(scymnus::has_type<init<nested_type>, properties>::value)
                                     v["properties"][f.name]["default"] = std::get<init<nested_type>>(f.properties).value();

                                 if constexpr(scymnus::has_type<constraints::max<nested_type>, properties>::value)
                                     v["properties"][f.name]["maximum"] = std::get<constraints::max<nested_type>>(f.properties).value();
                             }

                             else {
                                 //TODO: check min vs max
                                 if constexpr(scymnus::has_type<constraints::min<type>, properties>::value)
                                     v["properties"][f.name]["minimum"] = std::get<constraints::min<type>>(f.properties).value();

                                 if constexpr(scymnus::has_type<init<type>, properties>::value)
                                     v["properties"][f.name]["default"] = std::get<init<type>>(f.properties).value();

                                 if constexpr(scymnus::has_type<constraints::max<type>, properties>::value)
                                     v["properties"][f.name]["maximum"] = std::get<constraints::max<type>>(f.properties).value();
                             }






                             //TODO: clean code below

                             if constexpr (has_description<std::remove_const_t<decltype(f.properties)>>::value)
                             {
                                 constexpr int idx = tl::index_if<is_description,decltype(f.properties)>::value;
                                 v["properties"][f.name]["description"] =   std::get<idx>(f.properties).str();
                             }
                             else
                                 v["properties"][f.name]["description"] =  f.name;
                         }
                     }
                 }));
        return v;
    }
};






//describe enumeration type
template<class T,class... sl>
struct traits<enumeration<T,sl...>> {
    static json describe()
    {
        json v= traits<T>::describe();

        for (int i = 0; i < enumeration<T,sl...>::data().size(); i++){
            v["enum"][i] = enumeration<T,sl...>::data()[i];
        }

        v["description"] = "enum"; //swagger editor is not working correctly if no description is given
        return v;
    }
};


template<int Status, class T, class ResponseHeaders>
struct traits<response<Status, T, ResponseHeaders>> {


    static json describe()
    {
        json v;
        v["description"] = http_status_description<Status>();

        if constexpr (!std::is_same_v<std::remove_cvref_t<ResponseHeaders>,response_headers<>>)
            for_each(ResponseHeaders{}.headers_,[&](const auto& f){

                //TODO: Handle response descriptions
                using type = std::remove_cvref_t<decltype(f.get())>;
                json child = traits<type>::describe();
                v["headers"][f.name] = child;
            });


        auto ref = doc_types::get_type<std::decay_t<T>>();
        if (ref)
            v["schema"]["$ref"] = "#/definitions/" + ref.value();

        else{

            v["schema"]  = traits<std::decay_t<T>>::describe();
        }
        return v;
    }
};




template<int Status, class ResponseHeaders>
struct traits<response<Status, no_object, ResponseHeaders>> {


    static json describe()
    {
        json v;
        v["description"] = http_status_description<Status>();

        if constexpr (!std::is_same_v<std::remove_cvref_t<ResponseHeaders>,response_headers<>>)
            for_each(ResponseHeaders{}.headers_,[&](const auto& f){

                //TODO: Handle response descriptions
                using type = std::remove_cvref_t<decltype(f.get())>;
                json child = traits<type>::describe();
                v["headers"][f.name] = child;
            });

        return v;
    }
};



template<class T>
struct traits<std::map<std::string,T>> {
    static json describe()
    {
        json v;
        v["type"]="object";
        auto ref = doc_types::get_type<T>();
        if (ref)
            v["additionalProperties"]["$ref"] = "#/definitions/" + ref.get();
        return v;
    }
};

}//namespace scymnus



