#pragma once

#include <iostream>
#include <boost/intrusive_ptr.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "parser.hpp"
#include "buffer_pool.hpp"
#include "boost/asio/write.hpp"
#include "server/router.hpp"
#include "date_manager.hpp"





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



constexpr size_t page_size = 4096;


struct buffer_state
{
    char* watermark;
    uint8_t current_chunk;
    size_t capacity{page_size};

    bool new_buff{false};

};



class connection
{
    context ctx_;
    void* current_buffer;
public:


    connection(boost::asio::io_context& context) : socket_{context}
    {
#ifdef TEST_DEBUG
        std::cout << "connection created in thread:" << std::this_thread::get_id() << std::endl;
#endif

    }

    ~connection()
    {
#ifdef TEST_DEBUG
        std::cout << "connection destructed in thread:" << std::this_thread::get_id() << std::endl;
#endif
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void start(){
        current_buffer =  (buffer_manager::get().allocate());
        in_buffers_.emplace_back( static_cast<char*>(current_buffer));
        bs_.watermark = static_cast<char*>(current_buffer);


#ifdef TEST_DEBUG
        std::cout << "read called in thread:" << std::this_thread::get_id() << std::endl;
#endif
        read();
    }

    void read(){
        //TODO: check this
        if (bs_.capacity < 1){
            current_buffer =  (buffer_manager::get().allocate());
            in_buffers_.emplace_back( static_cast<char*>(current_buffer));

            bs_.watermark = static_cast<char*>(current_buffer);
            bs_.capacity = page_size;
            bs_.new_buff = true;

        }

        socket_.async_read_some(boost::asio::buffer(bs_.watermark, bs_.capacity),
                                [self = boost::intrusive_ptr(this), this](const boost::system::error_code& ec, std::size_t bytes_transferred)
                                {
                                    if (!ec)
                                    {
                                        //TODO: handle parsing errors
                                        llhttp_errno_t err = parser_.process(bs_.watermark, bytes_transferred);
                                        bs_.capacity -= bytes_transferred;
                                        bs_.watermark += bytes_transferred;

                                        if (err == HPE_OK) {
                                            if (parser_.state() != parser_state::MessageComplete)
                                            read();
                                        }
                                        else {
                                            //std::cout << "HTTP parsing error: " << parser_.get_error(err) << std::endl;
                                            ctx_.res.status_code = 400;
                                            keep_alive_ = false;
                                            complete_request();
                                        }
                                    }
                                    else{
                                       //close();
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



    void write_status_line(){
        //in case status code is not in status_codes map, the status line for status 500 will be written
        auto status = status_codes.at(ctx_.res.status_code.value_or(500), 500);
        buffers_.emplace_back(status.data(), status.size());
    }

    void write_headers(){
        static constexpr std::string_view seperator = ": ";
        static constexpr std::string_view crlf = "\r\n";

        for(auto& v : ctx_.res.headers)
        {
            buffers_.emplace_back(v.first.data(), v.first.size());
            buffers_.emplace_back(seperator.data(), seperator.size());
            buffers_.emplace_back(v.second.data(), v.second.size());
            buffers_.emplace_back(crlf.data(), crlf.size());
        }

        if (!ctx_.res.headers.count("content-length"))
        {
            static constexpr std::string_view content_length_key = "Content-Length: ";
            content_length_ = std::to_string(ctx_.res.body.size());
            buffers_.emplace_back(content_length_key.data(), content_length_key.size());
            buffers_.emplace_back(content_length_.data(), content_length_.size());
            buffers_.emplace_back(crlf.data(), crlf.size());
        }
        if (!ctx_.res.headers.count("server"))
        {
            static constexpr std::string_view server_key = "Server: ";

            buffers_.emplace_back(server_key.data(), server_key.size());
            buffers_.emplace_back(server_name.data(), server_name.size());
            buffers_.emplace_back(crlf.data(), crlf.size());
        }

        if (!keep_alive_){
            static constexpr std::string_view server_key = "Connection: close";
            buffers_.emplace_back(server_key.data(), server_key.size());
            buffers_.emplace_back(crlf.data(), crlf.size());
        }

        if (!ctx_.res.headers.count("date"))
        {
            static constexpr std::string_view date_key = "Date: ";
            date_str_ = date_manager::instance().get_date();

            buffers_.emplace_back(date_key.data(), date_key.size());
            buffers_.emplace_back(date_str_.data(), date_str_.size() - 1);
            buffers_.emplace_back(crlf.data(), crlf.size());
        }
        buffers_.emplace_back(crlf.data(), crlf.size());

    }


    void complete_request()
    {

        auto reset_context = finally([&] {
            ctx_ = context{};
        });


        if (!socket_.is_open())
        {
            return;
        }


        buffers_.clear();
        buffers_.reserve(4*(ctx_.res.headers.size()+5)+3);

        write_status_line();
        write_headers();

        buffers_.emplace_back(ctx_.res.body.data(), ctx_.res.body.size());
        do_write();



    }

    void do_write()
    {
#ifdef TEST_DEBUG
        std::cout << "write called in thread:" << std::this_thread::get_id() << std::endl;
#endif
        boost::asio::async_write(socket_, buffers_, [self = boost::intrusive_ptr(this), this]
                                 (const boost::system::error_code& ec, size_t) {
                                     if (ec) {
                                         //destructor will be called in this case
                                         //TODO: log error
                                         return;
                                     }

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

        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        is_closed_ = true;
    }



    bool is_closed_{false};
    bool keep_alive_{false};


    std::size_t ref_count_{0};
    parser<connection> parser_{this};
    friend class parser<connection>;
    boost::asio::ip::tcp::socket socket_;

    buffer_state  bs_;

    std::vector<boost::asio::const_buffer> buffers_;

    std::string content_length_;
    std::string date_str_;

    std::vector<std::unique_ptr<char,buffer_deleter>> in_buffers_;
};

} //namespace scymnus


