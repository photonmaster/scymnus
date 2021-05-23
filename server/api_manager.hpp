#pragma once

#include <unordered_map>
#include <typeindex>
#include <set>

#include "external/json.hpp"
#include "doc_traits.hpp"


namespace scymnus {
using json = nlohmann::json;


class api_manager
{

    struct info_object{
        std::string title{"Scymnus"};
        std::string description;
        std::string term_of_service;
        //contact info
        std::string name;
        std::string url;
        std::string email;
        //licence info
        std::string license_name;
        std::string license_url;
        std::string api_version{"1.0"};
    };


    info_object info_;
    std::string host_{"scymnus.gr"};
    std::string base_path_{"/"};
    std::vector<std::string> schemes_;
    std::vector<std::string> consumes_;
    std::vector<std::string> produces_;

    //TODO: tags can have a description
    std::set<std::string> tags_;

public:
    static api_manager& instance();


    void add_aspect_response(const std::string& aspect_name, int status, const json& description){

    //     swagger_["tags"] += v;


    json v;
    v["status"] = std::to_string(status);
    v["details"] = description;

    aspect_responses_[aspect_name] += v;

    }

    auto aspect_responses() const {
        return aspect_responses_;
    }



    std::string title() const{
        return info_.title;
    }

    void title(std::string title){
        info_.title = std::move(title);
    }

    std::string description() const{
        return info_.description;
    }

    std::string version() const
    {
        return info_.api_version;
    }

    void version(unsigned short major,unsigned short minor){
  //      info_.api_version = (boost::format{"%1%.%2%"} % major % minor).str();
    }

    void version(unsigned short major,unsigned short minor, unsigned short patch){
    //    boost::format("%1%.%2%.%3%") % major % minor ;
    //    d_pointer->info_.api_version = (boost::format{"%1%.%2%.%3%"} % major % minor % patch).str();
    }

    //term of service
    std::string tos() const{
        return info_.term_of_service;
    }


    void tos(std::string tos){
        info_.term_of_service = std::move(tos);
    }


    std::string name() const{
        return info_.name;
    }

    void name(std::string name){
        info_.name = std::move(name);
    }


    std::string url() const{
        return info_.url;
    }

    void url(std::string url){
        info_.url = std::move(url);
    }

    std::string email() const{
        return info_.email;
    }

    void email(std::string email){
        info_.email = std::move(email);
    }


    std::string license_name() const{
        return info_.license_name;
    }

    void license_name(std::string license_name){
        info_.license_name = std::move(license_name);
    }

    std::string license_url() const{
        return info_.license_url;
    }

    void license_url(std::string license_url){
        info_.license_url = std::move(license_url);
    }

     //TODO: remove this, by automating tag handling based on groups
    void add_tag(std::string name){
        tags_.insert(std::move(name));
    }


    std::string host() const{
        return host_;
    }

    void host(std::string host){
        host_ = std::move(host);
    }


    std::string base_path() const{
        return base_path_;
    }

    void base_path(std::string base_path){
        base_path_ = std::move(base_path);
    }


    void add_consume_type(std::string consume)
    {
        consumes_.push_back(std::move(consume));
    }

    void add_produce_type(std::string produce)
    {
        produces_.push_back(std::move(produce));
    }

    void add_scheme(std::string scheme)
    {
        schemes_.push_back(std::move(scheme));
    }


    //path to swagger-resources
    std::string swagger_path() const{
        return swagger_path_;
    }

    void swagger_path(std::string path)
    {
        swagger_path_ = (std::move(path));
    }


   inline std::string describe() {
        swagger_["swagger"] = "2.0";
        swagger_["host"]  =  host();
        swagger_["info"]["title"] = title();
        swagger_["info"]["version"] = version();


        swagger_["info"]["description"] = description();
        if (!license_name().empty()){
            swagger_["info"]["license"]["name"] = license_name();
            if (!license_url().empty()){
                swagger_["info"]["license"]["url"] = license_url();
            }
        }

        if (!name().empty())
            swagger_["info"]["contact"]["name"] = name();
        if (!email().empty())
            swagger_["info"]["contact"]["email"] = email();


        if (base_path() == "")
            base_path(version());

        swagger_["basePath"] = base_path();
        swagger_["consumes"] = consumes_;
        swagger_["produces"] = produces_;
        swagger_["schemes"]  = schemes_;

        for(const auto& tag  : tags_) {
            json v;
            v["name"] = tag;
            swagger_["tags"] += v;
        }


        if (doc_types::get_definitions().size()){
            swagger_["definitions"]  =  doc_types::get_definitions();
        }

        return api_manager::instance().swagger_.dump(3);
    }


    std::string swagger_description;
    json aspect_responses_;
    json endpoint_responses_;

    json swagger_;
    std::string swagger_path_;

private:
    friend class api;
    api_manager(const api_manager&) = delete;
    api_manager(api_manager&&) = delete;
    api_manager() = default;
};



//enum SecuritySchemaType{
//    ApiKey,
//    Basic,
//    OAuth2
//};



//class security_schema
//{
//public:
//    SecuritySchemaType type(){
//        return type_;
//    }

//    void type(SecuritySchemaType type){
//        type_ = type;
//    }

//    std::string description(){
//        return description_;
//    }

//    void description(const std::string& description){
//        description_ = description;
//    }



inline api_manager& api_manager::instance()
{
    static api_manager instance;
    return instance;
}



} //namespace scymnus
