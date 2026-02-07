#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <optional>

namespace project::core {

class RequestContext {
public:
    RequestContext() = default;

    // --- request id ---
    const std::string& request_id() const noexcept;
    void set_request_id(std::string id);

    // --- auth ---
    void set_user_id(std::string user_id);
    std::optional<std::string> user_id() const;

    // --- generic storage ---
    template<typename T>
    void set(std::string key, T value) {
        data_[std::move(key)] = std::any(std::move(value));
    }

    template<typename T>
    T* get(const std::string& key) {
        auto it = data_.find(key);
        if (it == data_.end()) {
            return nullptr;
        }
        return std::any_cast<T>(&it->second);
    }

private:
    std::string request_id_;
    std::optional<std::string> user_id_;
    std::unordered_map<std::string, std::any> data_;
};

} // namespace project::core

