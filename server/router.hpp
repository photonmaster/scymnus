#pragma once

#include <array>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/callable_traits.hpp>

#include "core/exception_handler.hpp"
#include "core/named_tuples_utils.hpp"
#include "core/opeartion.hpp"
#include "core/response.hpp"
#include "core/serializers.hpp"
#include "core/typelist.hpp"
#include "core/validator.hpp"
#include "http/http_common.hpp"

#include "aspects.hpp"
#include "external/json.hpp"
#include "http_context.hpp"
#include "utilities/utils.hpp"

namespace scymnus {

using json = nlohmann::json;
class node;

using callable_t = std::function<void(context &)>;
using match_result = node *; // std::pair<node*, std::vector<std::string_view>>;
namespace ct = boost::callable_traits;

enum class FragmentType : uint8_t {
    Simple,      // just a path segment
    Integer,     //:I
    Double,      //:D
    String,      //:S
    Regexp,      //:R
    PathWildCard // denoted as *
};

inline std::string to_string(FragmentType name) {
    if (name == FragmentType::Simple)
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
    node(FragmentType t = FragmentType::Simple) : type{t} {}
    node(FragmentType t, std::string v) : type{t}, value{v} {}

    struct node_comperator {
        bool operator()(const node *lhs, const node *rhs) const {

            if (lhs->type == rhs->type) {
                return lhs->value < rhs->value;
            }

            return lhs->type < rhs->type;
        }
    };

    FragmentType type{FragmentType::Simple};
    std::string value{};

    // std::set<node*, node_comperator> children{};

    std::vector<node *> children{};
    callable_t handler;
};

class trie {
    friend class router_manager;

public:
    [[nodiscard]] node *add(std::string url) {

        std::vector<std::string> parts;
        boost::trim_if(url,
                       boost::is_any_of("/")); // could also use plain boost::trim
        boost::split(parts, url, boost::is_any_of("/"), boost::token_compress_on);

        if (!parts.size())
            throw std::invalid_argument("url must not be empty");

        node *current_node = &head_;
        size_t index{};

        for (auto it = std::begin(parts); it != std::end(parts); ++it, ++index) {
            auto found = std::find_if(std::cbegin(current_node->children),
                                      std::cend(current_node->children),
                                      [&it](node *n) { return n->value == *it; });

            if (found != std::cend(current_node->children)) {
                current_node = *found;
                continue;
            }

            // PathWildCard is considered as  being the last type of the url
            if ((*it).compare(0, 2, ":*") == 0) {
                auto n = new node{FragmentType::PathWildCard, std::string{}};
                current_node->children.push_back(n);
                current_node = n;
                return current_node;
            }

            else if ((*it).compare(0, 3, ":R(") == 0 &&
                     it->back() == ')') { // Regular expression

                auto n =
                    new node{FragmentType::Regexp, (*it).substr(3, it->size() - 4)};
                current_node->children.push_back(n);
                current_node = n;
            }

            else if ((*it).compare(0, 2, ":S") == 0) { // String type

                auto n = new node{FragmentType::String, std::string{}};
                current_node->children.push_back(n);
                current_node = n;
            }

            else if ((*it).compare(0, 2, ":D") == 0) { // Double type
                auto n = new node{FragmentType::Double, std::string{}};
                current_node->children.push_back(n);
                current_node = n;
            }

            else if ((*it).compare(0, 2, ":I") == 0) { // Integer type
                auto n = new node{FragmentType::Integer, std::string{}};
                current_node->children.push_back(n);
                current_node = n;
            }

            else { // Simple type
                // check if node exists
                auto n = new node{FragmentType::Simple, *it};
                current_node->children.push_back(n);
                current_node = n;
            }
        }

        return current_node;
    }

    match_result match(const std::string_view url) {
        enum MatchType { NoMatch, PartialMatch, FullMatch } match_type{NoMatch};

        // TODO: check root
        auto path_start = url.find_first_not_of('/');
        auto path_end = url.find_first_of('/', path_start);

        node *current_node = &head_;

        while ((path_start != std::string::npos) ||
               (path_end != std::string::npos)) {
            std::string_view part = url.substr(
                path_start, path_end == std::string::npos ? url.size() - path_start
                                                          : path_end - path_start);

            for (auto iter = std::begin(current_node->children);
                 iter != std::end(current_node->children); iter++) {

                if ((*iter)->type == FragmentType::Simple) {
                    if (part == (*iter)->value) {
                        current_node = *iter;

                        match_type = PartialMatch;
                        break;
                    }
                    continue;
                }

                else if ((*iter)->type == FragmentType::Integer) {
                    match_type = PartialMatch;
                    for (const auto &c : part) {
                        if ((c >= '0' && c <= '9') || c == '+' ||
                            c == '-') { // TODO only allow + and - on the beginning
                            continue;
                        } else {
                            match_type = NoMatch;
                            break;
                        }
                    }
                    if (match_type == NoMatch)
                        continue;
                    current_node = *iter;
                    break; // we have match continue to the next fragment
                }

                else if ((*iter)->type == FragmentType::Double) {
                    match_type = PartialMatch;
                    for (const auto &c : part) {
                        if ((c >= '0' && c <= '9') || c == '.' || c == '+' ||
                            c == '-') { // TODO only allow + and - on the beginning
                            continue;
                        } else {
                            match_type = NoMatch;
                            break;
                        }
                    }
                    if (match_type == NoMatch)
                        continue;
                    current_node = *iter;
                    break; // we have match continue to the next fragment
                }

                else if ((*iter)->type == FragmentType::String) {
                    // we do have a match by defalut
                    current_node = *iter;
                    match_type = PartialMatch;
                    break; // we have match continue to the next fragment
                }

                // TODO: implement this
                else if ((*iter)->type == FragmentType::Regexp) {
                    match_type = PartialMatch;
                    current_node = *iter;
                    break;
                }

                else if ((*iter)->type == FragmentType::PathWildCard) {
                    match_type = PartialMatch;
                    current_node = *iter;
                    break;
                }
            }

            path_start = url.find_first_not_of('/', path_end);
            path_end = url.find_first_of('/', path_start);
        }

        return current_node;
    }

    trie() : head_{} {

        // method not found header
        head_.handler = [](context &ctx) { ctx.write(status<404>); };
    }

    trie(const trie &tr) = delete;

    // private:
    node head_;
};

template <class... P> class operation {
public:
    using parameters_t = scymnus::tl::typelist<P...>;
};

template <class L> struct unwrap;

template <template <class...> class L, class... A> struct unwrap<L<A...>> {
    using type = void(A...);
};

template <class T> struct unpacker;

template <class T, auto Path> struct param_visitor;

template <meta::ct_string Name, class T, meta::ct_string Path>
struct param_visitor<path_param<Name, T>, Path> {

    static constexpr const char *name = Name.str();
    using type = T;

    static decltype(auto) get(context &ctx) {
        std::vector<std::string_view> parts = utils::split(Path.str());
        std::string name_str = name;

        std::string target = "{" + name_str + "}";

        int counter = 0;
        for (auto t : parts) {
            if (target == std::string(t.data(), t.size()))
                break;
            ++counter;
        };

        return scymnus::path::traits<type>::get(ctx.raw_url(), counter);
    }
};

template <meta::ct_string Name, class T, meta::ct_string Path>
struct param_visitor<header_param<Name, T>, Path> {
    // Path is not used
    static decltype(auto) get(context &ctx) {
        static constexpr const char *name = Name.str();
        using type = T;
        return scymnus::header::traits<type>::get(ctx.request().headers(), name);
    }
};

template <meta::ct_string Name, class T, meta::ct_string Path>
struct param_visitor<query_param<Name, T>, Path> {
    static decltype(auto) get(context &ctx) {
        static constexpr const char *name = Name.str();
        using type = T;
        return scymnus::query::traits<type>::get(ctx.get_query_string(), name);
    }
};

template <meta::ct_string Name, class T, meta::ct_string Path>
struct param_visitor<body_param<Name, T>, Path> {
    static decltype(auto) get(context &ctx) {
        static constexpr const char *name = Name.str();
        json value = json::parse(ctx.request_body());
        T model = json::parse(ctx.request_body()).get<T>();
        return model;
    }
};

template <template <class...> class L, class... T> struct unpacker<L<T...>> {
    template <class F> static void execute(F &&f, context &ctx) {

        // get the path
        using return_type = ct::return_type_t<F>;
        constexpr meta::ct_string p = meta::ct_string(return_type::path);

        std::invoke(std::forward<F>(f),
                    T{param_visitor<T, return_type::path>::get(ctx)}..., ctx);
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

template <http_method Method, meta::ct_string Path, int c, class T>
class response_registrator {
public:
    response_registrator() {
        api_manager::instance().endpoint_responses_["paths"][std::string(
            Path)][to_string(Method)]["responses"][std::to_string(c)] =
                traits<T>::describe();

        if constexpr (T::content_type != http_content_type::NONE) {
            api_manager::instance().endpoint_produce_types_["paths"][std::string(
                Path)][to_string(Method)]["produces"] += describe(T::content_type);
        }
    }
};

template <http_method Method, meta::ct_string Path, int C, class T = void>
struct register_response {
    constexpr inline register_response() { &reg_; }
    //  static response_registrator<ct_string<meta::length(path)>{path}, c, T>
    //  reg_;
    static response_registrator<Method, Path, C, T> reg_;
};

template <http_method Method, meta::ct_string Path, int C, class T>
response_registrator<Method, Path, C, T>
    register_response<Method, Path, C, T>::reg_;

template <http_method Method, meta::ct_string Path> class response_for {
public:
    static constexpr meta::ct_string path = Path;
    static constexpr http_method method = Method;

    template <class T> inline constexpr response_for(T &&t) {
        (void)register_response<Method, Path, T::status, T>{};
    }

    inline constexpr response_for() = default;
};

template <class T>
using is_context = std::is_same<context, std::remove_cvref_t<T>>;

template <bool contains_context, class RT, class T> struct aspect_unpacker;

template <bool contains_context, class RT, template <class...> class L,
         class... T>
struct aspect_unpacker<contains_context, RT, L<T...>> {
    template <class F> static void execute(F &&f, context &ctx) {
        // get the path

        using return_type = RT;
        constexpr meta::ct_string p = meta::ct_string(return_type::path);
        if constexpr (contains_context) {
            std::invoke(std::forward<F>(f),
                        T{param_visitor<T, return_type::path>::get(ctx)}..., ctx);

        } else {
            std::invoke(std::forward<F>(f),
                        T{param_visitor<T, return_type::path>::get(ctx)}...);
        }
    }
};

struct router_parameters {
    std::string_view url_;
    http_method method_;
    std::string summary_{};
    std::string description_{};
    std::optional<std::string> tag_{}; // swagger tag

    router_parameters &description(const std::string &dsr) {
        description_ = dsr;
        return *this;
    }

    router_parameters &summary(const std::string &sum) {
        summary_ = sum;
        return *this;
    }

    router_parameters &tag(const std::string &tag) {
        tag_ = tag;
        return *this;
    }

    ~router_parameters() {

        std::string path(url_.data(), url_.size());

        std::string m = to_string(method_);

        api_manager::instance().swagger_["paths"][path][m]["summary"] = summary_;
        api_manager::instance().swagger_["paths"][path][m]["description"] =
            description_;
        api_manager::instance().swagger_["paths"][path][m]["operationId"] =
            to_string(method_) + '_' + path;

        if (tag_) {
            api_manager::instance().add_tag(*tag_);
            api_manager::instance().swagger_["paths"][path][m]["tags"] += *tag_;
        }
        if (!api_manager::instance()
                 .endpoint_responses_["paths"][path][m]
                 .is_null()) {
            api_manager::instance().swagger_["paths"][path][m]["responses"] =
                api_manager::instance()
                    .endpoint_responses_["paths"][path][m]["responses"];
        }

        if (!api_manager::instance()
                 .endpoint_produce_types_["paths"][path][m]
                 .is_null()) {
            api_manager::instance().swagger_["paths"][path][m]["produces"] =
                api_manager::instance()
                    .endpoint_produce_types_["paths"][path][m]["produces"];
        }
    }
};

class router {
public:
    router() = default;

    void exec(context &ctx) {
        try {

            // find path;
            auto path_start = ctx.raw_url().find('/');
            auto path_end = ctx.raw_url().find('?', path_start);

            std::string_view v = std::string_view(ctx.raw_url())
                                     .substr(path_start, path_end == std::string::npos
                                                             ? ctx.raw_url().size()
                                                             : path_end - path_start);
            method_data_[(std::size_t)ctx.method()].match(v)->handler(ctx);
        }

        catch (...) {
            exception_handler_(ctx);
        }
    }

private:
    template <class T>
    struct aspect_filter_t
        : tl::any_of<T, is_context, is_body_param, is_path_param> {};

    /// routes an endpoint without  updating swagger documentation and without
    /// using aspects
    template <class F> void route_internal(F &&f) {
        // get output parameters
        // TODO: check arguments
        using designated_types =
            tl::remove_if<is_context, ct::args_t<std::decay_t<F>, operation>,
                          operation<>>;

        json v = unpacker<designated_types>::describe();
        using return_type = ct::return_type_t<F>;

        // register aspect responses with endpoint
        // create the path for adding in trie

        using path_parameters =
            tl::select_if<is_path_or_segment_param,
                          ct::args_t<std::decay_t<F>, std::tuple>,
                          std::tuple<>>::type;
        std::string path = return_type::path.str();

        if constexpr (std::tuple_size_v<path_parameters>) {
            scymnus::for_each(path_parameters{}, [&path](const auto &v) {
                std::string str = "{" + std::string(v.name) + "}";

                if constexpr (is_segment_param_v<
                                  typename std::remove_cvref_t<decltype(v)>>) {
                    boost::replace_all(path, str, ":*");
                }

                else if constexpr (std::is_same_v<
                                       typename std::remove_cvref_t<decltype(v)>::type,
                                       int>) {
                    boost::replace_all(path, str, ":I");
                }

                else if (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,
                                        std::string>) {
                    boost::replace_all(path, str, ":S");

                }

                else if (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,
                                        double>) {
                    boost::replace_all(path, str, ":D");
                }
            });
        }

        auto l = [f = std::conditional_t<
                      std::is_lvalue_reference<F>::value,
                      std::reference_wrapper<std::remove_reference_t<F>>, F>{
                      std::forward<F>(f)}](context &ctx) {
            try {
                unpacker<typename designated_types::parameters_t>::execute(f, ctx);
            } catch (...) {
                // try to clean whatever is written in the response buffer
                ctx.clear();
                throw;
            }
        };

        auto node = method_data_[(std::size_t)return_type::method].add(path);
        node->handler = l;
    };

    template <class F, typename... T> router_parameters route(F &&f, T &&...t) {
        // get output parameters
        using designated_types =
            tl::remove_if<is_context, ct::args_t<std::decay_t<F>, operation>,
                          operation<>>;

        // we remove body_param types, path_param types and context type from
        // aspects so that they will not show up in swagger

        using union_t = tl::merge_t<
            designated_types,
            tl::remove_if<aspect_filter_t, ct::args_t<std::decay_t<T>, operation>,
                          operation<>>...>;

        json v = unpacker<typename union_t::parameters_t>::describe();
        using return_type = ct::return_type_t<F>;

        if (v.size())
            api_manager::instance().swagger_["paths"][std::string(
                return_type::path)][to_string(return_type::method)]["parameters"] = v;

        // register aspect responses wth endpoint

        static constexpr bool has_aspects = sizeof...(t);

        auto aspects = std::tuple<T...>{t...};

        for_each(aspects, [&](auto &aspect) {
            // get response type of aspect
            using aspect_return_type =
                ct::return_type_t<std::decay_t<decltype(aspect)>>;

            std::string aspect_name = aspect_return_type::aspect_name;

            if (api_manager::instance().aspect_responses().count(aspect_name)) {

                json response = api_manager::instance().aspect_responses()[aspect_name];

                //{"aspect_name":[{"details":{"description":"Not
                //Found","schema":{"type":"string"}},"status":404}]} iterate over the
                // list of resonses
                for (auto &it : response.items()) {
                    std::string status = it.value()["status"];
                    json details = it.value()["details"];
                    api_manager::instance().endpoint_responses_["paths"][std::string(
                        return_type::path)][to_string(return_type::method)]["responses"]
                                                               [status] = details;
                }
            }
        });

        // create the path for adding in Trie

        using path_parameters =
            tl::select_if<is_path_or_segment_param,
                          ct::args_t<std::decay_t<F>, std::tuple>,
                          std::tuple<>>::type;
        std::string path = return_type::path.str();

        if constexpr (std::tuple_size_v<path_parameters>) {
            scymnus::for_each(path_parameters{}, [&path](const auto &v) {
                std::string str = "{" + std::string(v.name) + "}";

                if constexpr (is_segment_param_v<
                                  typename std::remove_cvref_t<decltype(v)>>) {
                    boost::replace_all(path, str, ":*");
                }

                else if constexpr (std::is_same_v<
                                       typename std::remove_cvref_t<decltype(v)>::type,
                                       int>) {
                    boost::replace_all(path, str, ":I");
                }

                else if (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,
                                        std::string>) {
                    boost::replace_all(path, str, ":S");

                }

                else if (std::is_same_v<typename std::remove_cvref_t<decltype(v)>::type,
                                        double>) {
                    boost::replace_all(path, str, ":D");
                }
            });
        }

        // get before aspects. aspects with state must be supported

        auto before_aspects = std::apply(
            [](auto &&...t) {
                return std::tuple_cat([](auto &&arg) {
                    if constexpr (std::remove_cvref_t<decltype(arg)>::hook ==
                                  hook_type::before)
                        return std::tuple<decltype(arg)>{
                            std::forward<decltype(arg)>(arg)};
                    else
                        return std::tuple<>{};
                }(std::forward<decltype(t)>(t))...);
            },
            aspects);

        // get after aspects
        auto after_aspects = std::apply(
            [](auto &&...t) {
                return std::tuple_cat([](auto &&arg) {
                    if constexpr (std::remove_cvref_t<decltype(arg)>::hook ==
                                  hook_type::after)
                        return std::tuple<decltype(arg)>{
                            std::forward<decltype(arg)>(arg)};
                    else
                        return std::tuple<>{};
                }(std::forward<decltype(t)>(t))...);
            },
            aspects);

        auto l = [before_aspects = std::move(before_aspects),
                  after_aspects = std::move(after_aspects),
                  f = std::conditional_t<
                      std::is_lvalue_reference<F>::value,
                      std::reference_wrapper<std::remove_reference_t<F>>, F>{
                      std::forward<F>(f)}](context &ctx) {
            try {

                // execute before aspects

                if constexpr (has_aspects) {
                    for_each(before_aspects, [&](auto &a) {
                        if (ctx.is_response_written() && !a.is_mandatory)
                            return;

                        using aspect_arguments =
                            tl::remove_if<is_context,
                                          ct::args_t<std::decay_t<decltype(a)>, operation>,
                                          operation<>>;
                        constexpr bool has_context = tl::contains_type<
                            context &,
                            ct::args_t<std::decay_t<decltype(a)>, operation>>::value;

                        using return_type = ct::return_type_t<F>;
                        constexpr meta::ct_string p = meta::ct_string(return_type::path);

                        using aspect_return_type =
                            ct::return_type_t<std::decay_t<decltype(a)>>;
                        aspect_unpacker<has_context, return_type,
                                        aspect_arguments>::execute(a, ctx);
                    });
                }

                if constexpr (has_aspects) {
                    if (!ctx.is_response_written())
                        unpacker<typename designated_types::parameters_t>::execute(f, ctx);
                } else
                    unpacker<typename designated_types::parameters_t>::execute(f, ctx);

                // execute after aspects
                if constexpr (has_aspects) {
                    for_each(after_aspects, [&](auto &a) {
                        // No need to check if aspect is if aspect is mandatory

                        // the only signature allowed to aspects with hook_type::after is
                        // with zero argument or one argument
                        // of type const context&
                        using arguments_tuple = ct::args_t<std::decay_t<decltype(a)>>;
                        static_assert(sizeof(a) &&
                                          (std::is_same_v<arguments_tuple,
                                                          std::tuple<const context &>> ||
                                           std::is_same_v<arguments_tuple, std::tuple<>>),
                                      "aspects with hook_type::after "
                                      "(that are running after the main handler) must have "
                                      "no arguments or a single argument"
                                      "of type const context&");

                        using aspect_arguments =
                            tl::remove_if<is_context,
                                          ct::args_t<std::decay_t<decltype(a)>, operation>,
                                          operation<>>;
                        constexpr bool has_context = tl::contains_type<
                            const context &,
                            ct::args_t<std::decay_t<decltype(a)>, operation>>::value;

                        using return_type = ct::return_type_t<F>;
                        constexpr meta::ct_string p = meta::ct_string(return_type::path);

                        using aspect_return_type =
                            ct::return_type_t<std::decay_t<decltype(a)>>;
                        aspect_unpacker<has_context, return_type,
                                        aspect_arguments>::execute(a, ctx);
                    });
                }
            } catch (...) {
                // try to clean whatever is written in the response buffer
                ctx.clear();
                // call exceptions handler:
                exception_handler_(ctx);

                // should the mandatory after aspects run here?
            }
        };

        auto node = method_data_[(std::size_t)return_type::method].add(path);
        node->handler = std::move(l);

        return router_parameters{return_type::path.str(), return_type::method};
    }

    friend class app;

    static inline std::array<trie, (std::size_t)http_method::ENUM_MEMBERS_COUNT>
        method_data_{};

    // default exception handler
    static inline exception_handler exception_handler_{[](context &ctx) {
        try {
            throw;
        }

        catch (std::exception &exp) {
            std::string message = "default exception handler called: ";
            message.append(exp.what());
            ctx.write(status<400>, message);
        } catch (...) {
            std::string message =
                "default exception handler called, but exception is not known";
            ctx.write(status<400>, message);
        }
    }};
};

} // namespace scymnus
