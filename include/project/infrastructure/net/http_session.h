#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>

namespace project::infrastructure::net {

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    using Tcp = boost::asio::ip::tcp;
    using Request  = boost::beast::http::request<boost::beast::http::string_body>;
    using Response = boost::beast::http::response<boost::beast::http::string_body>;

    explicit HttpSession(Tcp::socket socket);

    // Точка входа — вызывается listener'ом
    void run();

private:
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes);

    void handle_request(Request&& req);

    void on_write(bool close,
                  boost::beast::error_code ec,
                  std::size_t bytes);

    void do_close();

private:
    Tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    Request request_;
};

} // namespace project::infrastructure::net

