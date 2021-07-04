#pragma once


#include "server/settings.hpp"
#include "api_manager.hpp"
#include "router.hpp"
#include "server.hpp"
#include "controllers/swagger_controller.hpp"



namespace scymnus {

class app{

public:
    static app& instance(){
        static app instance;

        return instance;
    }


    void run() {

        api_manager::instance().prepare_description();
        server_.run();
    }


    bool listen(std::string_view address, uint16_t port) {

        return server_.listen(address, port);
    }


    bool listen() {
        return server_.listen();
    }




    template <class F,  typename... T>
    router_parameters route(F &&f, T&&... t){
        return router_.route(f, t...);
    }

    template <class F,  typename... T>
    void route_internal(F &&f, T&&... t){
        return router_.route_internal(f, t...);
    }



    template<class Callable>
    void set_excpetion_handler(Callable&& callable){
        router_.exception_handler_.reset(std::forward<Callable>(callable));
    }



    void max_headers_size(uint16_t size) {
        server_.max_headers_size_ = size < 128?128:size;
    }

    uint16_t max_headers_size() const {
        return server_.max_headers_size_;
    }


    void max_url_size(uint16_t size) {
        server_.max_headers_size_ = size < 128?128:size;
    }

    uint16_t max_url_size() const {
        return server_.max_headers_size_;
    }


    void max_body_size(uint32_t size) {
        server_.max_body_size_ = size < 128?128:size;
    }

    uint32_t max_body_size() const {
        return server_.max_headers_size_;
    }

private:

    app(const app&) = delete;
    app(app&&) = delete;
    app() {
        route_internal(scymnus::get_swagger_description_controller{});
        route_internal(scymnus::api_doc_controller{});
        route_internal(scymnus::swagger_controller_files{});


    }


    scymnus::http_server<> server_{};
    router router_;



};


} //nameapace scymnus
