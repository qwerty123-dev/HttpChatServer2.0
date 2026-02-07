#pragma once

#include <string>
#include <unordered_map>

namespace project::core {

class HttpResponse {
public:
    using Headers = std::unordered_map<std::string, std::string>;

    HttpResponse();

    // --- status ---
    int status() const noexcept;
    void set_status(int status);

    // --- headers ---
    const Headers& headers() const noexcept;
    void set_header(std::string name, std::string value);

    // --- body ---
    const std::string& body() const noexcept;
    void set_body(std::string body);

    // --- helpers ---
    void set_json(std::string json_body);
    void set_text(std::string text_body);

private:
    int status_;
    Headers headers_;
    std::string body_;
};

} // namespace project::core

