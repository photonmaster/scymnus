#pragma once
#include <string>
#include <vector>
#include <string_view>

#include <boost/asio/ip/tcp.hpp>

#include "service_pool.hpp"

#include "connection.hpp"


namespace scymnus {

template<class service_pool_policy = service_pool>
class http_server {
public:
    template<class... Args>
    explicit http_server(Args&&... args) : pool_(std::forward<Args>(args)...)
    {
    }


    bool listen(std::string_view address, std::string_view port) {
        try {
            boost::asio::ip::tcp::resolver::query query(address.data(), port.data());
            boost::asio::ip::tcp::resolver resolver(pool_.next());
            boost::asio::ip::tcp::resolver::iterator endpoints = resolver.resolve(query);

            for (; endpoints != boost::asio::ip::tcp::resolver::iterator(); ++endpoints) {
                boost::asio::ip::tcp::endpoint endpoint = *endpoints;
                auto acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(pool_.next());
                acceptor->open(endpoint.protocol());
                acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

                acceptor->bind(endpoint);
                acceptor->listen();
                start_accept(acceptor);
                return true;
            }
        }
        catch (const std::exception& ex) {
            std::cout << ex.what() << "\n";
        }

        return false;
    }



    void stop() {
        pool_.stop();
    }

    void run() {
        pool_.run();
    }


private:

    void start_accept(std::shared_ptr<boost::asio::ip::tcp::acceptor> const& acceptor) {

        boost::asio::io_context&  context = pool_.next();
        boost::intrusive_ptr<connection> handler(new connection{context});

        acceptor->async_accept(handler->socket(), [&context,handler, acceptor,this](const boost::system::error_code& e) {

            if (!e) {
                boost::asio::post(context,[handler](){
                    handler->socket().set_option(boost::asio::ip::tcp::no_delay(true));
                    handler->start();

                });
            }
            else {
                //TODO: log error
            }
            start_accept(acceptor);

        });

    }

    service_pool_policy pool_;

};

} //namespace scymnus
