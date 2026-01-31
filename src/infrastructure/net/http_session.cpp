#include "project/infrastructure/net/http_session.h"

#include <boost/beast/http.hpp>
#include <iostream>

namespace project::infrastructure::net {

namespace http = boost::beast::http;

HttpSession::HttpSession(Tcp::socket socket)
    : socket_(std::move(socket)) {}

void HttpSession::run() {
    do_read();
}

void HttpSession::do_read() {
    request_ = {};

    http::async_read(
        socket_,
        buffer_,
        request_,
        boost::beast::bind_front_handler(
            &HttpSession::on_read,
            shared_from_this()));
}

void HttpSession::on_read(boost::beast::error_code ec, std::size_t) {
    if (ec == http::error::end_of_stream) {
        return do_close();
    }

    if (ec) {
        std::cerr << "[HttpSession] read error: " << ec.message() << std::endl;
        return;
    }

    handle_request(std::move(request_));
}

void HttpSession::handle_request(Request&& req) {
    Response res;
    res.version(req.version());
    res.keep_alive(req.keep_alive());

    // 🔹 ВРЕМЕННО: stub-ответ
    res.result(http::status::ok);
    res.set(http::field::server, "HttpChatServer2.0");
    res.set(http::field::content_type, "text/plain");
    res.body() = "Hello from HttpSession";
    res.prepare_payload();

    auto close = res.need_eof();

    http::async_write(
        socket_,
        res,
        boost::beast::bind_front_handler(
            &HttpSession::on_write,
            shared_from_this(),
            close));
}

void HttpSession::on_write(bool close,
                           boost::beast::error_code ec,
                           std::size_t) {
    if (ec) {
        std::cerr << "[HttpSession] write error: " << ec.message() << std::endl;
        return;
    }

    if (close) {
        return do_close();
    }

    do_read();
}

void HttpSession::do_close() {
    boost::beast::error_code ec;
    socket_.shutdown(Tcp::socket::shutdown_send, ec);
    // ignore error
}

} // namespace project::infrastructure::net

