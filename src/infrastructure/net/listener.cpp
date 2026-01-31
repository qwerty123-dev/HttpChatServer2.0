#include "project/infrastructure/net/listener.h"
#include "project/infrastructure/net/http_session.h"

#include <iostream>

namespace project::infrastructure::net {

Listener::Listener(
    boost::asio::io_context& io_context,
    const Tcp::endpoint& endpoint,
    SessionFactory session_factory
)
    : io_context_(io_context)
    , acceptor_(io_context)
    , session_factory_(std::move(session_factory))
{
    boost::system::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Listener: open failed: " + ec.message());
    }

    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        throw std::runtime_error("Listener: set_option failed: " + ec.message());
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        throw std::runtime_error("Listener: bind failed: " + ec.message());
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        throw std::runtime_error("Listener: listen failed: " + ec.message());
    }
}

void Listener::run() {
    if (stopped_) {
        return;
    }
    do_accept();
}

void Listener::stop() {
    bool expected = false;
    if (!stopped_.compare_exchange_strong(expected, true)) {
        return;
    }

    boost::system::error_code ec;
    acceptor_.cancel(ec);
    acceptor_.close(ec);
}

void Listener::do_accept() {
    acceptor_.async_accept(
        boost::asio::make_strand(io_context_),
        [self = shared_from_this()](boost::system::error_code ec,
                                    Tcp::socket socket) {
            if (self->stopped_) {
                return;
            }

            if (!ec) {
                try {
                    auto session = self->session_factory_(std::move(socket));
                    session->run();
                } catch (const std::exception&) {
                    // здесь позже будет логгер
                }
            }

            // продолжаем accept loop
            self->do_accept();
        }
    );
}

} // namespace project::infrastructure::net

