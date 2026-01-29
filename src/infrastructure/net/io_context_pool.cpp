#include <project/infrastructure/net/io_context_pool.h>
#include <stdexcept>

namespace project::infrastructure::net {

    IoContextPool::IoContextPool(std::size_t pool_size) {
        if (pool_size == 0) {
            throw std::invalid_argument("IoContextPool must be > 0");
        }
        contexts_.reserve(pool_size);

        for (std::size_t i{0}; i < pool_size; ++i) {
            auto io = std::make_unique<boost::asio::io_context>();
            auto guard = std::make_unique<WorkGuard>(
                boost::asio::make_work_guard(*io)
            );

            contexts_.emplace_back(ContextEntry{std::move(io), std::move(guard)});
        }
    }

    IoContextPool::~IoContextPool() {
        stop();
    }

    void IoContextPool::start() {
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true)) {
            return;
        }

        threads_.reserve(contexts_.size()); // Corrected typo here

        for (auto& entry : contexts_) {
            threads_.emplace_back([ctx = entry.io_context.get()] {
                try {
                    ctx->run();
                } catch (const std::exception& e) {
                    // Implement logging mechanism here
                    // e.g., logger.log(e.what());
                }
            });
        }
    }

    void IoContextPool::stop() {
        bool expected = true;
        if (!running_.compare_exchange_strong(expected, false)) {
            return;
        }

        for (auto& entry : contexts_) {
            entry.work_guard.reset(); // Ensure work_guard is reset
        }

        for (auto& entry : contexts_) {
            entry.io_context->stop();
        }

        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join(); // Fixed typo here
            }
        }

        threads_.clear();
    }

    boost::asio::io_context& IoContextPool::get_io_context() { // Fixed typo here
        auto index = next_.fetch_add(1, std::memory_order_relaxed);
        return *contexts_[index % contexts_.size()].io_context;
    }

    std::size_t IoContextPool::size() const noexcept {
        return contexts_.size();
    }

}
