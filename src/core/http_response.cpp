#include "project/core/http_response.h"

namespace project::core {

HttpResponse::HttpResponse()
    : status_(200) {}

int HttpResponse::status() const noexcept {
    return status_;
}

void HttpResponse::set_status(int status) {
    status_ = status;
}

const HttpResponse::Headers& HttpResponse::headers() const noexcept {
    return headers_;
}

void HttpResponse::set_header(std::string name, std::string value) {
    headers_[std::move(name)] = std::move(value);
}

const std::string& HttpResponse::body() const noexcept {
    return body_;
}

void HttpResponse::set_body(std::string body) {
    body_ = std::move(body);
}

void HttpResponse::set_json(std::string json_body) {
    set_header("Content-Type", "application/json");
    set_body(std::move(json_body));
}

void HttpResponse::set_text(std::string text_body) {
    set_header("Content-Type", "text/plain");
    set_body(std::move(text_body));
}

} // namespace project::core

