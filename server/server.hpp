#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "connection.hpp"
#include "server/settings.hpp"
#include "service_pool.hpp"

namespace scymnus {

template <class service_pool_policy = service_pool> class http_server {
public:
    template <class... Args>
    explicit http_server(Args &&...args) : pool_(std::forward<Args>(args)...) {}

    explicit http_server() : pool_(settings<core>()[CT_("workers")]) {}

    bool listen(std::string_view address, uint16_t port) {
        settings<core>()[CT_("ip")] = address;
        settings<core>()[CT_("port")] = port;

        try {
            boost::asio::ip::tcp::resolver::query query(address.data(),
                                                        std::to_string(port));
            boost::asio::ip::tcp::resolver resolver(pool_.next());
            boost::asio::ip::tcp::resolver::iterator endpoints =
                resolver.resolve(query);

            for (; endpoints != boost::asio::ip::tcp::resolver::iterator();
                 ++endpoints) {
                boost::asio::ip::tcp::endpoint endpoint = *endpoints;
                auto acceptor =
                    std::make_shared<boost::asio::ip::tcp::acceptor>(pool_.next());
                acceptor->open(endpoint.protocol());
                acceptor->set_option(
                    boost::asio::ip::tcp::acceptor::reuse_address(true));

                acceptor->bind(endpoint);
                acceptor->listen();
                start_accept(acceptor);
                return true;
            }
        } catch (const std::exception &ex) {
            throw;
        }

        return false;
    }

    bool listen() {
        return listen(settings<core>()[CT_("ip")], settings<core>()[CT_("port")]);
    }

    void stop() { pool_.stop(); }

    void run() { pool_.run(); }

private:
    friend class app;
    void start_accept(
        std::shared_ptr<boost::asio::ip::tcp::acceptor> const &acceptor) {

        boost::asio::io_context &context = pool_.next();

        boost::asio::post(context, [&context, acceptor, this]() {
            boost::intrusive_ptr<connection> handler(new connection{context});

            acceptor->async_accept(handler->socket(),
                                   [&context, handler, acceptor,
                                    this](const boost::system::error_code &e) {
                                       start_accept(acceptor);
                                       if (!e) {

                                           handler->socket().set_option(
                                               boost::asio::ip::tcp::no_delay(true));
                                           handler->idle_timeout_setup(
                                               settings<core>()[CT_("idle_timeout")]);
                                           handler->read();
                                       } else {
                                           // TODO: log error
                                       }
                                   });
        });
    }

    service_pool_policy pool_;

    // idle timeout in seconds
    uint32_t idle_timeout_{60};
    // request max sizes in bytes

    uint16_t max_headers_size_{8 * 1024};
    uint16_t max_url_size_{2 * 1024};
    uint32_t max_body_size_{8 * 1024};
};

} // namespace scymnus
