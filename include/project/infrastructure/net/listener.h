#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <functional>

namespace project::infrastructure::net {

class HttpSession;

class Listener : public std::enable_shared_from_this<Listener> {
public:
    using Tcp = boost::asio::ip::tcp;
    using SessionFactory =
        std::function<std::shared_ptr<HttpSession>(Tcp::socket)>;

    Listener(
        boost::asio::io_context& io_context,
        const Tcp::endpoint& endpoint,
        SessionFactory session_factory
    );

    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    // Запуск accept loop
    void run();

    // Остановка acceptor (graceful)
    void stop();

private:
    void do_accept();

    boost::asio::io_context& io_context_;
    Tcp::acceptor acceptor_;
    SessionFactory session_factory_;
    std::atomic<bool> stopped_{false};
};

} // namespace project::infrastructure::net

