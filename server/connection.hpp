#pragma once

#include <iostream>
#include <boost/intrusive_ptr.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "parser.hpp"
#include "buffer_pool.hpp"
#include "boost/asio/write.hpp"
#include "server/router.hpp"
#include "date_manager.hpp"
#include "external/decimal_from.hpp"


#include <memory_resource>

namespace scymnus
{

namespace detail{

template <typename F>
struct final_action {
    final_action(F f) : clean_{f} {

    }
    ~final_action() {
        if(enabled_) clean_();
    }
    void disable() {
        enabled_ = false;
    };
private:
    F clean_;
    bool enabled_{true};
};

}

template <typename F>
detail::final_action<F> finally(F f) {
    return detail::final_action<F>(f); }



constexpr size_t page_size = 2 * 10 * 1000;


struct buffer_state
{
    char* watermark;
    uint8_t current_chunk;
    size_t capacity{page_size};

    bool new_buff{false};

};



template <typename T>
class parser;

class connection
{
    context ctx_{};
    char* current_buffer;
public:


    connection(boost::asio::io_context& context)
        : socket_{context},
          timer_{context}
    {
#ifdef TEST_DEBUG
        std::cout << "connection created in thread:" << std::this_thread::get_id() << std::endl;
#endif
        set_timer(idle_timeout_);
    }

    ~connection()
    {
        cancel_timer();
#ifdef TEST_DEBUG
        std::cout << "connection destructed in thread:" << std::this_thread::get_id() << std::endl;
#endif
    }

    //called on first use
    void idle_timeout_setup(uint32_t seconds){
        if (seconds){
            last_activity_tp_ = std::chrono::steady_clock::now();
            idle_timeout_ = seconds;
            set_timer(seconds);
        }
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void start(){

        current_buffer =  (buffer_manager::get().allocate());
        in_buffers_.emplace_back(current_buffer);
        bs_.watermark = current_buffer;


#ifdef TEST_DEBUG
        std::cout << "read called in thread:" << std::this_thread::get_id() << std::endl;
#endif
        read();
    }

    void read(){
        //TODO: check this
        if (bs_.capacity < 128){
            current_buffer =  (buffer_manager::get().allocate());
            in_buffers_.emplace_back(current_buffer);

            bs_.watermark = current_buffer;
            bs_.capacity = page_size;
            bs_.new_buff = true;

        }
#ifdef TEST_DEBUG
        std::cout << "read called in thread:" << std::this_thread::get_id() << std::endl;
#endif

        last_activity_tp_ = std::chrono::steady_clock::now();

        socket_.async_read_some(boost::asio::buffer(bs_.watermark, bs_.capacity),
                                [self = boost::intrusive_ptr(this), this](const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            if (!ec)
            {
                //TODO: handle parsing errors
                llhttp_errno_t err = parser_.process(bs_.watermark, bytes_transferred);
                bs_.capacity -= bytes_transferred;
                bs_.watermark += bytes_transferred;

                last_activity_tp_ = std::chrono::steady_clock::now();

                if (err == HPE_OK) {

                    if (parser_.state() != parser_state::MessageComplete)
                        read();
                }
                else {
#ifdef TEST_DEBUG
                    std::cout << "HTTP parsing error: " << parser_.get_error(err) << std::endl;
#endif
                    ctx_.res.status_code = 400;
                    keep_alive_ = false;
                    complete_request();
                }
            }
            else{
                //dtor will be called closing the connection
            }
        }
        );
    }


    void check_keep_alive(){

        auto header_value = ctx_.req.get_header_value("connection");

        if (parser_.minor_version() == 1){
            if (header_value && iequals(*header_value,"close"))
                keep_alive_ = false;
            else
                keep_alive_ = true;
        }
        else if (parser_.minor_version() == 0){
            if (header_value && iequals(*header_value,"keep-alive"))
                keep_alive_ = true;
            else
                keep_alive_ = false;
        }
    }



    void exec()
    {
        check_keep_alive();
        router{}.exec(ctx_);
        complete_request();
    };



    void complete_request()
    {

        response.clear();

        static constexpr std::string_view success_response = "HTTP/1.1 200 OK\r\n";

        if (ctx_.res.status_code.value() == 200)
            response.append("HTTP/1.1 200 OK\r\n");
        else{
            auto status = status_codes.at(ctx_.res.status_code.value_or(500), 500);
            response.append(status);
        }

        for(auto& v : ctx_.res.headers)
        {
            response.append(v.first);
            response.append(":");
            response.append(v.second);
            response.append("\r\n");
        }

        static constexpr auto long_bufsize  = std::numeric_limits<long>::max_digits10 + 2;
        char spec[long_bufsize];
        content_length_ = decimal_from(ctx_.res.body.size(), spec);//   std::to_string(ctx_.res.body.size());


        response.append("Content-Length:");
        response.append(content_length_);
        response.append("\r\n");


        if (!keep_alive_){
            response.append( "Connection:close\r\n");
        }

        date_str_ = date_manager::instance().get_date();

        response.append("Date:");
        response.append(date_str_);
        response.append(" GMT\r\n\r\n");
        response.append(ctx_.res.body);

        do_write();
    }

    void do_write()
    {
#ifdef TEST_DEBUG
        std::cout << "write called in thread:" << std::this_thread::get_id() << std::endl;
#endif
        boost::asio::async_write(socket_, boost::asio::buffer(response)/*buffers_*/, [self = boost::intrusive_ptr(this), this]
                                 (const boost::system::error_code& ec, size_t) {
            if (ec) {
                //destructor will be called in this case
                return;
            }
            last_activity_tp_ = std::chrono::steady_clock::now();

            reset();

            if (keep_alive_) {
                read();
            }
        });
    }


    //intrusive_ptr handling

    inline friend void intrusive_ptr_add_ref(connection* c) noexcept{
        ++c->ref_count_;
    }

    inline friend void intrusive_ptr_release(connection* c) noexcept{
        if(--c->ref_count_ == 0)
            delete c;
    }
private:

    void close(){
        if (is_closed_)
            return;

        cancel_timer();
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        is_closed_ = true;
    }

    void set_timer(uint32_t seconds) {
        if (!idle_timeout_)
            return;

        timer_.expires_from_now(std::chrono::seconds(seconds));
        timer_.async_wait([this](boost::system::error_code const &ec) {
            if (ec) {
                return;
            }
            auto now =  std::chrono::steady_clock::now();

            if (last_activity_tp_ + std::chrono::seconds(idle_timeout_) < now){

                auto f = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_tp_);
                set_timer(f.count());

            }
            else{
                close();
            }
        });
    }


    void cancel_timer() {
        if (!idle_timeout_)
            return;
        boost::system::error_code ec;
        timer_.cancel(ec);
    }

    void reset() {
        ctx_.reset();
        in_buffers_.clear();
        header_size_ = 0;
    }

    bool is_closed_{false};
    bool keep_alive_{false};


    std::size_t ref_count_{0};

    friend class parser <connection>;
    parser<connection> parser_{this};

    boost::asio::ip::tcp::socket socket_;
    buffer_state  bs_;


    std::string content_length_;
    std::string date_str_;

    std::vector<std::unique_ptr<char,buffer_deleter>> in_buffers_;

    uint32_t idle_timeout_{0};
    boost::asio::steady_timer timer_;
    std::chrono::time_point<std::chrono::steady_clock> last_activity_tp_;
    //std::chrono::time_point<std::chrono::steady_clock> idle_time_tp_{std::chrono::seconds(idle_timeout_) + last_activity_tp_};


    std::string response;
    uint16_t header_size_{0};
};

} //namespace scymnus


