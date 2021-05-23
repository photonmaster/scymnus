#pragma once

#include <utility>  //for pair
#include <string_view>
#include <thread>

#include "external/http_parser/llhttp.h"

#include "headers_container.hpp"
#include "url/url.hpp"
#include "http/http_common.hpp"
#include "server/app.hpp"


namespace scymnus
{

//TODO: remove
namespace parsing_flags {
enum state : uint8_t {
    None      = 0,
    HasContentLength = 1 << 0, // 00001 == 1
    CloseConnection  = 1 << 1, // 00010 == 2
};
}

inline constexpr parsing_flags::state operator| (parsing_flags::state a, parsing_flags::state b) {
    return a = static_cast<parsing_flags::state> (a | b);
}

inline constexpr parsing_flags::state operator& (parsing_flags::state a, parsing_flags::state b) {
    return a = static_cast<parsing_flags::state> (a & b);
}




/** Simple state model used to track the parsing of HTTP headers */
enum class parser_state : uint8_t {
    Init,
    Url,
    HeaderField,
    HeaderValue,
    HeadersComplete,
    Body,
    TrailerField,
    TrailerValue,
    MessageComplete
};




template<class Session>
class  parser{
private:

    llhttp_t parser_{};
    Session* session_;
    std::pair<message_data_t,message_data_t> header_;
    // headers_container headers_;
    std::string_view url_;  //TODO: what if url_ is not contiguous?

    parser_state parser_state_{parser_state::Init};

public:

    parser(Session* session) : session_{session}{

#ifdef TEST_DEBUG
        std::cout << "parser called in thread:" << std::this_thread::get_id() << std::endl;
#endif
        llhttp_init(&parser_, HTTP_REQUEST,  &settings_);
        parser_.data = this;


    }

    parser_state  state(){
        return  parser_state_;
    }


    static int on_message_begin(llhttp_t* parser){
        return HPE_OK;

    }


    static int on_message_complete(llhttp_t* llhttp){
#ifdef TEST_DEBUG
        std::cout << "on_message_complete called in thread:" << std::this_thread::get_id() << std::endl;
#endif
        parser<Session>* self = static_cast<parser<Session>*>(llhttp->data);

        //TODO: check if method is not supprted
        //route and call handler

        self->session_->exec();
        self->parser_state_ = parser_state::MessageComplete;
        return HPE_OK;
    }



    static int on_chunk_header(llhttp_t* parser){
        return HPE_OK;

    }


    static int on_chunk_complete(llhttp_t* parser){
        return HPE_OK;


    }


    static int on_url(llhttp_t* llhttp, const unsigned char *at, size_t length){
        parser<Session>* self = static_cast<parser<Session>*>(llhttp->data);

        self->session_->ctx_.method = self->method();
        self->session_->ctx_.raw_url = std::string{reinterpret_cast<const char*>(at), length};

        //create uri object
        self->session_->ctx_.url = scymnus::uri<>{self->session_->ctx_.raw_url};
        self->parser_state_ = parser_state::Url;

        return HPE_OK;
       // return HPE_USER;
    }


    static int on_body(llhttp_t* llhttp, const unsigned char *at, size_t length){
        parser<Session>* self = static_cast<parser<Session>*>(llhttp->data);

        //check if we are in bo

        auto sv = std::string_view{reinterpret_cast<const char*>(at), length};

        if(self->parser_state_ == parser_state::Body)
        {
            if (self->session_->bs_.new_buff){
                //read data are not contiguous
                if (std::holds_alternative<std::string_view>(self->session_->ctx_.req.body)){
                    string_view_cluster<char> cluster;
                    cluster.append(std::get<std::string_view>(self->session_->ctx_.req.body));
                    //append new data
                    cluster.append(sv);
                    self->session_->ctx_.req.body = std::move(cluster);
                }
                else {
                    std::get<string_view_cluster<char>>(self->session_->ctx_.req.body).append(sv);
                }

                self->session_->bs_.new_buff = false;
            }

            else{
                if (std::holds_alternative<std::string_view>(self->session_->ctx_.req.body)){
                    //expand string_view no need for introducing a cluster yet
                    std::string_view current =  std::get<0>(self->session_->ctx_.req.body);
                    self->session_->ctx_.req.body = std::string_view{current.data(), length + current.size()};
                }

                else {
                    // a cluster has been already created
                    std::get<1>(self->session_->ctx_.req.body).append(sv);

                }
            }

        }
        else {
            //reading body for the first time
            self->session_->ctx_.req.body = sv;

        }
        self->parser_state_ = parser_state::Body;
        return HPE_OK;
    }


    static int on_status(llhttp_t *, const unsigned char *at, size_t length){
        return HPE_OK;

    }


    static int on_header_field(llhttp_t * llhttp, const unsigned char *at, size_t length){
#ifdef TEST_DEBUG
        std::cout << "on header field" << std::endl;
#endif
        parser<Session>* self = static_cast<parser<Session>*>(llhttp->data);

        if (self->parser_state_ == parser_state::HeaderValue){
            //a field-value pair is ready

#ifdef TEST_DEBUG
            std::cout << "adding header: ";
            std::cout << "adding field: " << self->header_.first << " value: " << self->header_.second << std::endl;
#endif
            self->session_->ctx_.req.headers.emplace(std::move(self->header_.first), std::move(self->header_.second));
            self->header_ = {};
        }

        //we are in the middle of a field read: push back a fragment. (unlikely)
        if (self->parser_state_ == parser_state::HeaderField){
            //adjust header value
            std::string_view sv =  std::get<0>(self->header_.first);
            self->header_.first = std::string_view{sv.data(), length + sv.size()};
        }

        else
        {
            self->header_.first = std::string_view{reinterpret_cast<const char*>(at), length};
            self->parser_state_ = parser_state::HeaderField;
        }
        return HPE_OK;
    }

    static int on_header_value(llhttp_t * llhttp, const unsigned char *at, size_t length){
#ifdef TEST_DEBUG
        std::cout << "on header value" << std::endl;
#endif
        auto self = static_cast<parser<Session>*>(llhttp->data);
        self->header_.second = std::string_view{reinterpret_cast<const char*>(at), length};

        self->parser_state_ = parser_state::HeaderValue;
        return HPE_OK;
    }

    static int on_headers_complete(llhttp_t* llhttp){
#ifdef TEST_DEBUG
        std::cout << "on headers complete" << std::endl;
#endif
        auto self = static_cast<parser<Session>*>(llhttp->data);
        //add last header
        self->session_->ctx_.req.headers.emplace(std::move(self->header_.first), std::move(self->header_.second));

#ifdef TEST_DEBUG
        std::cout << "total number of headers: " << self->session_->ctx_.req.headers.size();
        for (const auto& f: self->session_->ctx_.req.headers){
            std::cout << "field: " << f.first << " value: " << f.second << std::endl;
        }
#endif

        self->parser_state_ = parser_state::HeadersComplete;
        return HPE_OK;
    }


    static constexpr llhttp_settings_t settings_ {
        on_message_begin,
        on_url,
        on_status,
        on_header_field,
        on_header_value,
        on_headers_complete,
        on_body,
        on_message_complete,
        on_chunk_header,
        on_chunk_complete
    };





public:



    llhttp_errno_t process(const char* buffer, int length)
    {
        return llhttp_execute(&parser_, buffer, length);
    }


    uint16_t major_version() const {
        return parser_.http_major;
    }


    uint16_t minor_version() const {
        return parser_.http_minor;
    }

    std::string get_error(llhttp_errno_t err) const {
        return llhttp_errno_name(err);
    }

    http_method method(){
        //            HTTP_DELETE = 0,
        //            HTTP_GET = 1,
        //            HTTP_HEAD = 2,
        //            HTTP_POST = 3,
        //            HTTP_PUT = 4,
        //            HTTP_CONNECT = 5,
        //            HTTP_OPTIONS = 6,
        //            HTTP_TRACE = 7,
        //            HTTP_COPY = 8,
        //            HTTP_LOCK = 9,
        //            HTTP_MKCOL = 10,
        //            HTTP_MOVE = 11,
        //            HTTP_PROPFIND = 12,
        //            HTTP_PROPPATCH = 13,
        //            HTTP_SEARCH = 14,
        //            HTTP_UNLOCK = 15,
        //            HTTP_BIND = 16,
        //            HTTP_REBIND = 17,
        //            HTTP_UNBIND = 18,
        //            HTTP_ACL = 19,
        //            HTTP_REPORT = 20,
        //            HTTP_MKACTIVITY = 21,
        //            HTTP_CHECKOUT = 22,
        //            HTTP_MERGE = 23,
        //            HTTP_MSEARCH = 24,
        //            HTTP_NOTIFY = 25,
        //            HTTP_SUBSCRIBE = 26,
        //            HTTP_UNSUBSCRIBE = 27,
        //            HTTP_PATCH = 28,
        //            HTTP_PURGE = 29,
        //            HTTP_MKCALENDAR = 30,
        //            HTTP_LINK = 31,
        //            HTTP_UNLINK = 32,
        //            HTTP_SOURCE = 33

        if (parser_.method < HTTP_COPY)
            return static_cast<http_method>(parser_.method);
        else if(parser_.method == HTTP_PATCH)
            return http_method::PATCH;
        else {
            return http_method::ENUM_MEMBERS_COUNT;
        }

    }
};



} //namespace scymnus
