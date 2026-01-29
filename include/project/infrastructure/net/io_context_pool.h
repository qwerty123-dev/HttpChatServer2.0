#pragma once 

#include <boost/asio/io_context.hpp>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

namespace project::infrastructure::net {
    class IoContextPool {
        public:
            explicit IoContextPool(std::size_t pool_size);
            ~IoContextPool();
            IoContextPool(const IoContextPool&) = delete;
            IoContextPool& operator=(const IoContextPool&) = delete;
            void start();
            void stop();
            boost::asio::io_context& get_io_context();  // Fixed typo here
            std::size_t size() const noexcept;

        private:
            using IoContextPtr = std::unique_ptr<boost::asio::io_context>;  // Fixed type name
            using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

            struct ContextEntry {
                IoContextPtr io_context;
                std::unique_ptr<WorkGuard> work_guard;
            };

            std::vector<ContextEntry> contexts_;
            std::vector<std::thread> threads_;
            std::atomic<std::size_t> next_{ 0 };  // Added missing semicolon
            std::atomic<bool> running_{ false };
    };
}
