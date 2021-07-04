#pragma once

#include "http/http_common.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http/query_parser.hpp"
#include "url/url.hpp"


namespace scymnus
{

struct context
{
    http_method method;
    std::string raw_url;
    std::string patterned_url;
    std::optional< std::vector<std::string_view>> path_details;
    uri<> url;
    std::optional<query_string> query;


    http_request  req;
    http_response  res;


    ~context(){
    }
    void reset(){
        req.reset();
        res.reset();
        end_ = false;
        query.reset();
        url = {};
        path_details.reset();
        patterned_url.clear();
        raw_url.clear();
    }

    bool is_ended(){
        return end_;
    }
    void end(){
        end_ = true;
    }

    //write support
    query_string get_query_string(){
        if(query)
            return query.value();
        query = query_string();
        if(!url.query().empty()){
            query.value().parse(url.query());
        }
        return query.value();
    }

private:
    bool end_{false};

};

} //namespace scymnus
