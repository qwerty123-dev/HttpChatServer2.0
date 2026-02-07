#pragma once

#include <string>
#include <unordered_map>
#include <string_view>

namespace project::core {

class HttpRequest {
public:
    using Headers = std::unordered_map<std::string, std::string>;
    using QueryParams = std::unordered_map<std::string, std::string>;

    HttpRequest() = default;

    // --- method / path ---
    const std::string& method() const noexcept;
    const std::string& path() const noexcept;

    void set_method(std::string method);
    void set_path(std::string path);

    // --- headers ---
    const Headers& headers() const noexcept;
    bool has_header(std::string_view name) const;
    std::string header(std::string_view name) const;

    void set_header(std::string name, std::string value);

    // --- query params ---
    const QueryParams& query_params() const noexcept;
    std::string query_param(std::string_view key) const;
    void set_query_param(std::string key, std::string value);

    // --- body ---
    const std::string& body() const noexcept;
    void set_body(std::string body);

private:
    std::string method_;
    std::string path_;
    Headers headers_;
    QueryParams query_params_;
    std::string body_;
};

} // namespace project::core

