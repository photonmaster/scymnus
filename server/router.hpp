#pragma once

#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <type_traits>
#include <thread>
#include <string_view>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/callable_traits.hpp>

#include "http/http_common.hpp"
#include "core/opeartion.hpp"
#include "core/serializers.hpp"
#include "core/typelist.hpp"
#include "core/named_tuples_utils.hpp"
#include "core/response.hpp"
#include "core/validator.hpp"
#include "core/exception_handler.hpp"

#include "external/json.hpp"
#include "http_context.hpp"
#include "utilities/utils.hpp"
#include "aspects.hpp"



namespace scymnous {

namespace detail {
struct tuple_sink {
    template <typename T>
    tuple_sink(T&&){

    }

    operator std::tuple<>() {
        return {};
    }

};



}

//REF: https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c/56766138#56766138

template <typename T>
constexpr auto type_name() noexcept {
    std::string_view name = "Error: unsupported compiler", prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void) noexcept";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}




using json = nlohmann::json;
class node;
using callable_t = std::function<void(context&)>;
using match_result = std::pair<node*, std::vector<std::string_view>>;


namespace ct = boost::callable_traits;

enum class FragmentType : uint8_t {
    Simple,            // just a path segment
    Integer,           //:I
    Double,            //:D
    String,            //:S
    Regexp,            //:R
    PathWildCard       // denoted as *
};




inline std::string to_string(FragmentType name){
    if  (name == FragmentType::Simple)
        return "Simple";
    else if (name == FragmentType::Double)
        return "double";
    else if (name == FragmentType::Regexp)
        return "Regexp";
    else if (name == FragmentType::String)
        return "String";
    else if (name == FragmentType::Integer)
        return "Integer";
    else if (name == FragmentType::PathWildCard)
        return "PathWildCard";
    else
        return "Unknown";
}

struct node {
    node(FragmentType t = FragmentType::Simple) :
        type{t}
    {
        id = counter++;
    }
    node(FragmentType t, std::string v) :
        type{t},
        value{v}
    {
        id = counter++;
    }


    struct node_comperator {
        bool operator()(const node *lhs, const node *rhs) const {

            if (lhs->type == rhs->type){
                return lhs->value < rhs->value;
            }

            return lhs->type < rhs->type;
        }
    };

    FragmentType type{FragmentType::Simple};
    std::string value{};
    std::set<node*, node_comperator> children{};
    int id{};

    //std::variant<double,string,int> path_value;


    static inline int counter{0};

};





class trie {
    friend class router_manager;
public:
    [[nodiscard]] node* add(std::string url) {
        //#ifdef TEST_DEBUG
        //        std::cout << "add(url) starting. head children size:  " << head_.children.size() << std::endl;
        //#endif

        std::vector<std::string> parts;
        boost::trim_if(url, boost::is_any_of("/")); // could also use plain boost::trim
        boost::split(parts, url, boost::is_any_of("/"), boost::token_compress_on);

#ifdef TEST_DEBUG
        for (size_t i = 0; i < parts.size(); i++) {
            std::cout << "part[" << i << "]: " << parts[i] << std::endl;
        }
#endif
        if (!parts.size()) throw std::invalid_argument("url must not be empty");


        node *current_node = &head_;
        size_t index{};

        for (auto it = std::begin(parts); it != std::end(parts); ++it, ++index) {
#ifdef TEST_DEBUG
            std::cout << "value of it = " << *it << std::endl;
#endif
            auto found = std::find_if(std::cbegin(current_node->children),
                                      std::cend(current_node->children), [&it](node* n){return n->value == *it;});

            if (found != std::cend(current_node->children)){
                current_node = *found;
                continue;
            }


            if ((*it).compare(0, 2, ":*") == 0) {
                auto n = new node{FragmentType::PathWildCard, std::string{}};
                const auto &reply = current_node->children.insert(n);
                current_node = &(**reply.first);
            }

            else if ((*it).compare(0, 3, ":R(") == 0 &&
                     it->back() == ')') {  // Regular expression

                auto n = new node{FragmentType::Regexp, (*it).substr(3, it->size() - 4)};
                const auto &reply = current_node->children.insert(n);
                current_node = &(**reply.first);
            }

            else if ((*it).compare(0, 2, ":S") == 0) {  // String type

                auto n = new node{FragmentType::String, std::string{}};
                const auto &reply = current_node->children.insert(n);
                current_node = &(**reply.first);
            }

            else if ((*it).compare(0, 2, ":D") == 0) {  // Double type
                auto n = new node{FragmentType::Double,std::string{}};
                const auto &reply = current_node->children.insert(n);
                current_node = &(**reply.first);
            }

            else if ((*it).compare(0, 2, ":I") == 0) {  // Integer type
                auto n = new node{FragmentType::Integer, std::string{}};
                const auto &reply = current_node->children.insert(n);
                current_node = &(**reply.first);
            }

            else {  // Simple type
                //check if node exists
                auto n = new node{FragmentType::Simple, *it};
                current_node->children.insert(n);
                current_node = n;

#ifdef TEST_DEBUG
                std::cout << "current node: value: " << current_node->value;
#endif
            }
        }


#ifdef TEST_DEBUG
        std::cout << "about to return the last node with type: " << to_string( current_node->type) << std::endl;
#endif
        return current_node;
    }



    match_result match(const std::string_view url) {

        std::vector<std::string_view> parts = utils::split(url);  // skip empty parts

#ifdef TEST_DEBUG
        for (size_t i = 0; i < parts.size(); i++) {
            std::cout << "part[" << i << "]: " << parts[i] << std::endl;
        }
#endif

        enum MatchType {NoMatch, PartialMatch, FullMatch} match_type{NoMatch};

        size_t index{0};
        node *current_node = &head_;
        node *wild_card_node{nullptr};

#ifdef TEST_DEBUG
        std::cout << "id of head node is: " << current_node->id << std::endl;
        std::cout << "head node child size: " << current_node->children.size() << std::endl;
#endif

        std::vector<std::string_view> path_parameters;

        for (auto it = std::begin(parts); it != std::end(parts); it++, index++) {
            match_type = NoMatch;
            // iterate on child nodes
            std::cout << "checking part: " << (*it) << std::endl;

            if (wild_card_node){
                path_parameters.back() = std::string_view{path_parameters.back().data(),path_parameters.back().size() + parts[index].size() + 1};
            }
            for (auto iter = std::begin(current_node->children);
                 iter != std::end(current_node->children); iter++) {
#ifdef TEST_DEBUG
                std::cout << "value of it: " << (*iter)->value << std::endl;
#endif
                if ((*iter)->type == FragmentType::Simple) {
                    if (parts[index] == (*iter)->value) {
                        current_node = *iter;
#ifdef TEST_DEBUG
                        std::cout << "partial match found" << std::endl;
#endif
                        match_type = PartialMatch;
                        wild_card_node = nullptr;
                        break;
                    }

                    continue;
                }

                else if ((*iter)->type == FragmentType::Integer) {
                    match_type = PartialMatch;
                    for (const auto &c : parts[index]) {
                        if ((c >= '0' && c <= '9') || c == '+' ||
                            c == '-') {  // TODO only allow + and - on the beginning
                            continue;
                        } else {
                            match_type = NoMatch;
                            break;
                        }
                    }
                    if (match_type == NoMatch) continue;

                    wild_card_node = nullptr;
                    current_node = *iter;
                    path_parameters.push_back(parts[index]);


                    break;  // we have match continue to the next fragment
                }

                else if ((*iter)->type == FragmentType::Double) {
                    match_type = PartialMatch;
                    for (const auto &c : parts[index]) {
                        if ((c >= '0' && c <= '9') || c == '.' || c == '+' ||
                            c == '-') {  // TODO only allow + and - on the beginning
                            continue;
                        } else {
                            match_type = NoMatch;
                            break;
                        }
                    }
                    if (match_type == NoMatch) continue;
                    path_parameters.push_back(parts[index]);
                    wild_card_node = nullptr;
                    current_node = *iter;

                    break;  // we have match continue to the next fragment
                }

                else if ((*iter)->type == FragmentType::String) {
                    // we do have a match by defalut
                    current_node = *iter;
                    match_type = PartialMatch;
                    path_parameters.push_back(parts[index]);
                    wild_card_node = nullptr;
                    break;  // we have match continue to the next fragment
                }

                // TODO: implement this
                else if ((*iter)->type == FragmentType::Regexp) {
                    match_type = PartialMatch;
                    current_node = *iter;
                    path_parameters.push_back(parts[index]);
                    wild_card_node = nullptr;
                    break;
                }

                else if ((*iter)->type == FragmentType::PathWildCard) {
#ifdef TEST_DEBUG
                    std::cout << "FragmentType::PathWildCard found" << std::endl;
#endif
                    match_type = PartialMatch;
                    path_parameters.emplace_back(parts[index].data(), parts[index].size());
                    wild_card_node = current_node;
                    current_node = *iter;
                    break;
                }
                if (!wild_card_node) {
                    return {nullptr, path_parameters};
                }
            }

        }

        if (match_type == PartialMatch || wild_card_node)
        {

#ifdef TEST_DEBUG
            std::cout << "out of match(). Returning node with id:  " << current_node->id << std::endl;

            std::cout << "path segments: " << std::endl;
            for (size_t i = 0; i < path_parameters.size(); i++) {
                std::cout << "path_parameters[" << i << "]: " << path_parameters[i] << std::endl;
            }
#endif
            return {current_node, path_parameters};

        }
        return {nullptr, path_parameters};
    }

    trie() : head_{}{
        std::cout << "trie ctor" << std::endl;
    }

    trie(const trie& tr) = delete;

    ~trie() {
    }

    //private:
    node head_;
};

template <class... P>
class operation {
public:
    using parameters_t = scymnous::tl::typelist<P...>;
};

template <class L>
struct unwrap;

template <template <class...> class L, class... A>
struct unwrap<L<A...>> {
    using type = void(A...);
};

template <class T>
struct unpacker;




template<class T,  auto Path>
struct param_visitor;

//template<meta::ct_string Path>
//struct param_visitor<context, Path>
//{
//    //Path is not used
//    static decltype(auto) get(context& ctx)
//    {
//        return ctx;
//    }
//};


template<ct_string Name, class  T, meta::ct_string Path>
struct param_visitor<path_param<Name,T>, Path>
{

    static constexpr const char* name = Name.str();
    using type = T;

    static decltype(auto) get(context& ctx)
    {

#ifdef TEST_DEBUG

        std::cout << "param_visitor for path param called" << std::endl;

        std::cout << "From path param visitor: \n";
        std::cout << "patterned url: " << ctx.patterned_url << '\n';
        std::cout << "param_visitor for path param called" << std::endl;

        if (ctx.path_details){
            const auto& pd = ctx.path_details.value();
            std::cout << "path_details found: \n";
            for (size_t i = 0; i < pd.size(); i++) {
                std::cout << "path_details[" << i << "]: " << pd[i] << std::endl;
            }
        }
#endif

        std::vector<std::string_view> parts = utils::split(Path.str());
        std::string name_str =  name;

        std::string target = "{" + name_str + "}";

        int counter = 0;
        for (auto t : parts) {
            if (target == std::string(t.data(), t.size())) break;
            ++counter;
        };

        return scymnous::path::traits<type>::get(ctx.raw_url,counter);
    }
};



template<ct_string Name, class  T, meta::ct_string Path>
struct param_visitor<header_param<Name,T>, Path>
{
    //Path is not used
    static decltype(auto) get(context& ctx)
    {
        static constexpr const char* name = Name.str();
        using type = T;
        return scymnous::header::traits<type>::get(ctx.req.headers,name);
    }
};


template<ct_string Name, class  T, meta::ct_string Path>
struct param_visitor<query_param<Name,T>, Path>
{
    static decltype(auto) get(context& ctx)
    {
        static constexpr const char* name = Name.str();
        using type = T;
        return scymnous::query::traits<type>::get(ctx.get_query_string(),name);
    }
};




template<ct_string Name, class  T, meta::ct_string Path>
struct param_visitor<body_param<Name,T>, Path>
{
    static decltype(auto) get(context& ctx)
    {
        static constexpr const char* name = Name.str();
        using type = T;
        type model;

        //TODO: check  if body is not contiguous and use to_string because currently nlohamann does not support incremental parsing
        json value = json::parse(std::get<0>(ctx.req.body));

        from_json(value,model);
        return model;
    }
};

template<ct_string Name, meta::ct_string Path>
struct param_visitor<segment_param<Name>, Path>
{
    static constexpr const char* name = Name.str();

    static std::string get(context& ctx)
    {


#ifdef TEST_DEBUG

        std::cout << "param_visitor for path param called" << std::endl;

        std::cout << "From path param visitor: \n";
        std::cout << "patterned url: " << ctx.patterned_url << '\n';
        std::cout << "param_visitor for path param called" << std::endl;

        if (ctx.path_details){
            const auto& pd = ctx.path_details.value();
            std::cout << "path_details found: \n";
            for (size_t i = 0; i < pd.size(); i++) {
                std::cout << "path_details[" << i << "]: " << pd[i] << std::endl;
            }
        }
#endif

        std::vector<std::string_view> parts = utils::split(Path.str());
        std::string name_str =  name;

        std::string target = "{" + name_str + "}";

        int counter = 0;
        int pattern_position = 0;

        for (auto t : parts) {

            if (target == std::string(t.data(), t.size())) break;
            if (*(t.data()) == '{' || *(t.data()) == ':') ++pattern_position;
            ++counter;
        };

        auto sv =  ctx.path_details.value()[pattern_position];

        return std::string(sv.data(), sv.size());
    }
};


template <template <class...> class L, class... T>
struct unpacker<L<T...>> {
    template<class F>
    static void execute(F&& f, context& ctx){

#ifdef TEST_DEBUG

        if (ctx.path_details){
            const auto& pd = ctx.path_details.value();
            std::cout << "path_details found: \n";
            for (size_t i = 0; i < pd.size(); i++) {
                std::cout << "path_details[" << i << "]: " << pd[i] << std::endl;
            }
        }
#endif

        //get the path
        using return_type = ct::return_type_t<F>;
        constexpr  meta::ct_string p = meta::ct_string(return_type::path);

        std::apply(std::forward<F>(f), std::forward_as_tuple(T{param_visitor<T, return_type::path>::get(ctx)}...,ctx));
    }


    static json describe() {
        json descr;
        auto describe_ = [&descr](const auto &t) {
            descr += std::decay_t<decltype(t)>::describe();
        };
        (void)(int[]){0, (describe_(T{}), 0)...};
        return descr;
    }
};




///response_headers holds headers that must be appear in a response




//is response trait

template<typename>
struct is_response : std::false_type {};

template<int Status, class T>
struct is_response<response<Status,T>> : std::true_type {};


template<http_method Method,meta::ct_string Path, int c, class T>
class response_registrator {
public:
    response_registrator() {
        api_manager::instance().endpoint_responses_["paths"][std::string(Path)][to_string(Method)]["responses"][std::to_string(c)] =
            traits<T>::describe();
    }
};



template<http_method Method, meta::ct_string Path, int C, class T = void>
struct register_response {
    constexpr inline register_response() {
        &reg_;
    }
    //  static response_registrator<ct_string<meta::length(path)>{path}, c, T> reg_;
    static response_registrator<Method,Path, C, T> reg_;
};


template<http_method Method,meta::ct_string Path, int C, class T>
response_registrator<Method, Path, C, T> register_response<Method, Path, C, T>::reg_;



template<http_method Method, meta::ct_string Path>
class response_for
{
public:
    static constexpr meta::ct_string path = Path;
    static constexpr http_method method = Method;

    template<class T>
    inline constexpr response_for(T&& t)
    {
        (void)register_response<Method, Path, T::status, T>{};
    }

    inline constexpr response_for() = default;
};


template <class T>
using is_context = std::is_same<context, std::remove_cvref_t<T>>;


template <bool contains_context, class RT, class T>
struct aspect_unpacker;


template <bool contains_context,class RT, template <class...> class L, class... T>
struct aspect_unpacker<contains_context,RT,L<T...>> {
    template<class F>
    static void execute(F&& f, context& ctx){
        //get the path

        using return_type = RT;
        constexpr  meta::ct_string p = meta::ct_string(return_type::path);
        if constexpr (contains_context){
            std::apply(std::forward<F>(f), std::forward_as_tuple(T{param_visitor<T, return_type::path>::get(ctx)}...,ctx));

        }
        else{
            std::apply(std::forward<F>(f), std::forward_as_tuple(T{param_visitor<T, return_type::path>::get(ctx)}...));
        }


    }
};


struct router_parameters {
    std::string_view url_;
    http_method method_;
    std::string summary_{};
    std::string description_{};
    std::optional<std::string> tag_{}; //swagger tag

    router_parameters& description(const std::string& dsr){
        description_ = dsr;
        return *this;
    }

    router_parameters& summary(const std::string& sum){
        summary_ = sum;
        return *this;
    }


    router_parameters& tag(const std::string& tag){
        tag_ = tag;
        return *this;
    }


    ~router_parameters(){

        std::string path(url_.data(), url_.size());

        std::string m = to_string(method_);


        api_manager::instance().swagger_["paths"][path][m]["summary"] = summary_;
        api_manager::instance().swagger_["paths"][path][m]["description"] = description_;
        api_manager::instance().swagger_["paths"][path][m]["operationId"] = to_string(method_) + '_' + path;


        if(tag_){
            api_manager::instance().add_tag(*tag_);
            api_manager::instance().swagger_["paths"][path][m]["tags"] += *tag_;

            if (!api_manager::instance().endpoint_responses_["paths"][path][m].is_null()) {
                std::cout << "responses for " << path << ": " <<
                    api_manager::instance().endpoint_responses_["paths"][path][m]["responses"].dump() << std::endl;
                api_manager::instance().swagger_["paths"][path][m]["responses"] =
                    api_manager::instance().endpoint_responses_["paths"][path][m]["responses"];
            }
        }

        //                if (!consumes_.empty())
        //                    api_manager::instance().swagger["paths"][path][operation_to_string<type>()]["consumes"]
        //                        = consumes_;
        //                if (!produces_.empty())
        //                    api_manager::instance().swagger["paths"][path][operation_to_string<type>()]["produces"]
        //                        = produces_;

        //                if (!tags_.empty()) {
        //                    api_manager::instance().swagger["paths"][path][operation_to_string<type>()]["tags"]
        //                        = tags_;
        //                }

        //     api_manager::instance().swagger["paths"][path][operation_to_string<type>()]["responses"] = responses();

    }
};


class router {
public:

    router() = default;
    void match(context& ctx){
        try {

#ifdef TEST_DEBUG
            std::cout << "match called in thread:" << std::this_thread::get_id() << std::endl;
            std::cout << "about to match method: " << scymnous::to_string(ctx.method) << std::endl;
            std::cout << "index is:" << (size_t)ctx.method << std::endl;
#endif

            auto match_data =  method_data_[ctx.method].trie_.match(ctx.url.path());


            if (!match_data.first){
                ctx.res.status_code = 404;
            }
            else {
                if (method_data_[ctx.method].controllers_.count(match_data.first)){
                    ctx.path_details = std::move(match_data.second);
                    method_data_[ctx.method].controllers_.at(match_data.first)(ctx);
                }
                else {
                    std::cout << "controller not found" << std::endl;
                    ctx.res.status_code = 404;
                }
            }
        }

        catch(...){
            exception_handler_(ctx);
        }
    }



private:

    template<class T> struct aspect_filter_t : tl::any_of<T, is_context,is_body_param,is_path_param>{};


    /// routes an endpoint without  updating swagger documentation
    template <class F,  typename... T>
    void route_internal(F &&f, T&&... t)
    {
        //get output parameters
        //TODO: check arguments
        using designated_types = tl::remove_if<is_context,ct::args_t<std::decay_t<F>, operation>, operation<>>;
        using union_t = tl::merge_t<designated_types, tl::remove_if<is_context,ct::args_t<std::decay_t<T>, operation>, operation<>>...>;

        json v = unpacker<typename union_t::parameters_t>::describe();
        using return_type = ct::return_type_t<F>;


        //register aspect responses with endpoint
        //create the path for adding in trie

        using path_parameters = tl::select_if<is_path_or_segment_param,ct::args_t<std::decay_t<F>, std::tuple>, std::tuple<>>::type;
        std::string path = return_type::path.str();

        if constexpr (std::tuple_size_v<path_parameters>){
            scymnous::for_each(path_parameters{}, [&path](const auto& v){
                std::string str = "{" + std::string(v.name) + "}";

                if constexpr (is_segment_param_v<typename std::remove_cvref_t<decltype(v)>>){
                    boost::replace_all(path, str, ":*");
                }


                else if constexpr (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,int>){
                    boost::replace_all(path, str, ":I");
                }

                else if   (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,std::string>){
                    boost::replace_all(path, str, ":S");

                }

                else if   (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,double>){
                    boost::replace_all(path, str, ":D");

                }
            });
        }


        auto l = [patterned_url = path, aspects = std::tuple<T...>{t...},  f = std::conditional_t<
                                                                              std::is_lvalue_reference<F>::value,
                                                                              std::reference_wrapper<std::remove_reference_t<F>>, F>{
                                                                              std::forward<F>(f)}](context& ctx) {
            ctx.patterned_url = patterned_url;

            //execute pre
            bool stop_processing = false;

            for_each(aspects, [&](auto& a){
                if (stop_processing) return;

                using aspect_arguments = tl::remove_if<is_context,ct::args_t<std::decay_t<decltype(a)>, operation>, operation<>>;
                constexpr bool has_context =  tl::contains_type<context&, ct::args_t<std::decay_t<decltype(a)>, operation>>::value;

                using return_type = ct::return_type_t<F>;
                constexpr  meta::ct_string p = meta::ct_string(return_type::path);

                using aspect_return_type =
                    ct::return_type_t<std::decay_t<decltype(a)>>;

                aspect_unpacker<has_context, return_type, aspect_arguments>::execute(a,ctx);
            });


            //check this only when aspects are present
            if (stop_processing) return;

            unpacker<typename designated_types::parameters_t>::execute(f,ctx);
            //execute post
        };




        auto node =  method_data_[return_type::method].
                    trie_.add(path);
        method_data_[return_type::method].controllers_[node] = l;

    };


    template <class F,  typename... T>
    router_parameters route(F &&f, T&&... t){
        //get output parameters
        //TODO: check arguments
        using designated_types = tl::remove_if<is_context,ct::args_t<std::decay_t<F>, operation>, operation<>>;


        //we remove body_param types, path_param types and context type from aspects
        //so that they will not show up in swagger

        using union_t = tl::merge_t<
            designated_types,
            tl::remove_if<aspect_filter_t,
                          ct::args_t<std::decay_t<T>, operation>, operation<>>...>;

        json v = unpacker<typename union_t::parameters_t>::describe();
        using return_type = ct::return_type_t<F>;


        if (v.size())
            api_manager::instance().swagger_["paths"][std::string(return_type::path)][to_string(return_type::method)]["parameters"] = v;

        //register aspect responses wth endpoint
        auto aspects = std::tuple<T...>{t...};



        for_each(aspects, [&](auto& aspect){
            //get response type of aspect
            using aspect_return_type =
                ct::return_type_t<std::decay_t<decltype(aspect)>>;

            std::string aspect_name =  aspect_return_type::aspect_name;

            if (api_manager::instance().aspect_responses().count(aspect_name)){

                json response = api_manager::instance().aspect_responses()[aspect_name];

                //{"aspect_name":[{"details":{"description":"Not Found","schema":{"type":"string"}},"status":404}]}
                //iterate over the list of resonses
                for (auto& it : response.items())
                {
                    std::string status =   it.value()["status"];
                    json details = it.value()["details"];
                    api_manager::instance().endpoint_responses_["paths"][std::string(return_type::path)][to_string(return_type::method)]["responses"][status] =
                        details;
                }
            }
        });

        //create the path for adding in Trie

        using path_parameters = tl::select_if<is_path_or_segment_param,ct::args_t<std::decay_t<F>, std::tuple>, std::tuple<>>::type;
        std::string path = return_type::path.str();

        if constexpr (std::tuple_size_v<path_parameters>){
            scymnous::for_each(path_parameters{}, [&path](const auto& v){
                std::string str = "{" + std::string(v.name) + "}";

                if constexpr (is_segment_param_v<typename std::remove_cvref_t<decltype(v)>>){
                    boost::replace_all(path, str, ":*");
                }


                else if constexpr (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,int>){
                    boost::replace_all(path, str, ":I");
                }

                else if   (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,std::string>){
                    boost::replace_all(path, str, ":S");

                }

                else if   (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,double>){
                    boost::replace_all(path, str, ":D");

                }
            });
        }


        //get before aspects. aspects with state must be supported


        auto before_aspects = std::apply([]( auto&...t) {

            return std::tuple_cat([](auto& arg){
                if constexpr (std::remove_cvref_t<decltype(arg)>::hook == hook_type::before)
                    return  std::tuple<decltype(arg)>{arg};
                else
                return std::tuple<>{};
            }(t)...);
        }, aspects);


       //get after aspects
        auto after_aspects = std::apply([]( auto&...t) {

            return std::tuple_cat([](auto& arg){
                if constexpr (std::remove_cvref_t<decltype(arg)>::hook == hook_type::after)
                    return  std::tuple<decltype(arg)>{arg};
                else
                    return std::tuple<>{};
            }(t)...);
        }, aspects);

        auto l = [patterned_url = path,
                  before_aspects = std::move(before_aspects),
                  after_aspects = std::move(after_aspects),
                  f = std::conditional_t<std::is_lvalue_reference<F>::value,
                                         std::reference_wrapper<std::remove_reference_t<F>>, F>{
                      std::forward<F>(f)}](context& ctx) {
            ctx.patterned_url = patterned_url;

            //execute before aspects
   //         if constexpr (std::tuple_size<decltype(before_aspects)>::value){
                for_each(before_aspects, [&](auto& a){

                    using aspect_arguments = tl::remove_if<is_context,ct::args_t<std::decay_t<decltype(a)>, operation>, operation<>>;
                    constexpr bool has_context =  tl::contains_type<context&, ct::args_t<std::decay_t<decltype(a)>, operation>>::value;

                    using return_type = ct::return_type_t<F>;
                    constexpr  meta::ct_string p = meta::ct_string(return_type::path);

                    using aspect_return_type =
                        ct::return_type_t<std::decay_t<decltype(a)>>;
                    aspect_unpacker<has_context, return_type, aspect_arguments>::execute(a,ctx);
                });
            //}

            unpacker<typename designated_types::parameters_t>::execute(f,ctx);

            //execute after aspects
            for_each(after_aspects, [&](auto& a){

                using aspect_arguments = tl::remove_if<is_context,ct::args_t<std::decay_t<decltype(a)>, operation>, operation<>>;
                constexpr bool has_context =  tl::contains_type<context&, ct::args_t<std::decay_t<decltype(a)>, operation>>::value;

                using return_type = ct::return_type_t<F>;
                constexpr  meta::ct_string p = meta::ct_string(return_type::path);

                using aspect_return_type =
                    ct::return_type_t<std::decay_t<decltype(a)>>;
                aspect_unpacker<has_context, return_type, aspect_arguments>::execute(a,ctx);
            });
        };


        auto node =  method_data_[return_type::method].
                    trie_.add(path);
        method_data_[return_type::method].controllers_[node] = l;

        return router_parameters{return_type::path.str(), return_type::method};
    }

    friend class app;

    using method_data =
        struct
    {
        std::unordered_map<node*, callable_t> controllers_;
        trie trie_;
    };

    static inline std::unordered_map<http_method, method_data> method_data_{};


    //default exception handler
    static inline exception_handler exception_handler_ {
        [](context& ctx){
            std::cout << "default exception handler called" << std::endl;

            try{
                throw;
            }

            catch(std::exception& exp){
                ctx.res.status_code = 400;
                ctx.res.body = exp.what();
            }
        }
    };



};

}  // namespace scymnous
