#pragma once

#include <exception>
#include <array>
#include <boost/lexical_cast.hpp>

#include "server/http_context.hpp"
#include "external/json.hpp"

namespace scymnous {

using json = nlohmann::json;


//TODO:add constraints of what must be in here
template<class... H>
struct response_headers {
    std::tuple<H...> headers_ {};

    constexpr response_headers() = default;



    //TODO: why this must be public?
    template<class... Headers>
    constexpr response_headers(std::tuple<Headers...>&& tuple)  : headers_{std::move(tuple)} {
    }

    template<class U>
    [[nodiscard/*("a new object is returned when adding a header")*/]]
    auto constexpr add(U&& header){
        return response_headers<H..., U> {std::tuple_cat(headers_, std::make_tuple(std::forward<U>(header)))};
    }

    static auto create(){
        return response_headers<>{};
    }

    ///utility function that sets the headers in response-headers object
    /// in the response

    static void fill_headers(const response_headers& headers, context& ctx)
    {

        for_each(headers.headers_,[&](auto& f){
            //static constexpr const char* name = f.str();
            //TODO: set constraints to possible header_param values
            ctx.res.add_header(f.name, f.get());

        });
    }


};





template <int Status> using status_t = std::integral_constant<int, Status>;
template <int Status> constexpr status_t<Status> status;

struct no_object{};

template<int Status, class T = no_object, class ResponseHeaders = response_headers<>>
class response
{
public:
    static constexpr int status = Status;
    using type = T;
    using response_headers_t = ResponseHeaders;


    //TODO: add constraints in ResponseHeaders

    response(status_t<Status> st, const T& t, const ResponseHeaders& headers, context& ctx)
    {
        if (ctx.res.status_code){
            return;
        }
        using nlohmann::to_json;
        json v;
        to_json(v, t);
        ctx.res.status_code = st;
        ctx.res.add_header("Content-Type", "application/json");
        ResponseHeaders::fill_headers(headers,ctx);
        ctx.res.body = v.dump();
    }
    response(status_t<Status> st, const T& t, context& ctx)
    {

        if (ctx.res.status_code){
            return;
        }
        using nlohmann::to_json;
        json v;
        to_json(v, t);
        ctx.res.add_header("Content-Type", "application/json");
        ctx.res.status_code = st;
        ctx.res.body = v.dump();

    }


    response(status_t<Status> st, context& ctx)
    {
        if (ctx.res.status_code){
            return;
        }
        ctx.res.add_header("Content-Type", "application/json");
        ctx.res.status_code = st;
    }
};



//specialised response for json object

template<int Status, class ResponseHeaders>
class response<Status, json, ResponseHeaders>
{
public:
    static constexpr int status = Status;
    using type = json;
    using response_headers_t = ResponseHeaders;


    //TODO: add constraints in ResponseHeaders

    response(status_t<Status> st, const json& t, const ResponseHeaders& headers, context& ctx)
    {

        //if a response has been set already return
        if (ctx.res.status_code){
            return;
        }

        ctx.res.status_code = st;
        ResponseHeaders::fill_headers(headers,ctx);
        ctx.res.add_header("Content-Type", "application/json");
        ctx.res.body = t.dump();
    }


    response(status_t<Status> st, const json& t, context& ctx)
    {
        if (ctx.res.status_code){
            return;
        }
        ctx.res.status_code = st;
        ctx.res.body = t.dump();
    }
};











////response object
////TODO: change class name
////TODO: handle absense of response type
//template<int C,class T>
//class resp
//{
//public:
//    static constexpr int code = C;
//    using type = T::type;
//    using response_headers_t = T::response_headers_t;


////    void description(std::string description){
////        description_ = std::move(description);
////    }

////    std::string description() const
////    {
////        return description_;
////    }

//};

} //namespace scymnous
