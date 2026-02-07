#include "project/core/http_request.h"

namespace project::core {

const std::string& HttpRequest::method() const noexcept {
    return method_;
}

const std::string& HttpRequest::path() const noexcept {
    return path_;
}

void HttpRequest::set_method(std::string method) {
    method_ = std::move(method);
}

void HttpRequest::set_path(std::string path) {
    path_ = std::move(path);
}

const HttpRequest::Headers& HttpRequest::headers() const noexcept {
    return headers_;
}

bool HttpRequest::has_header(std::string_view name) const {
    return headers_.find(std::string(name)) != headers_.end();
}

std::string HttpRequest::header(std::string_view name) const {
    auto it = headers_.find(std::string(name));
    return it != headers_.end() ? it->second : std::string{};
}

void HttpRequest::set_header(std::string name, std::string value) {
    headers_[std::move(name)] = std::move(value);
}

const HttpRequest::QueryParams& HttpRequest::query_params() const noexcept {
    return query_params_;
}

std::string HttpRequest::query_param(std::string_view key) const {
    auto it = query_params_.find(std::string(key));
    return it != query_params_.end() ? it->second : std::string{};
}

void HttpRequest::set_query_param(std::string key, std::string value) {
    query_params_[std::move(key)] = std::move(value);
}

const std::string& HttpRequest::body() const noexcept {
    return body_;
}

void HttpRequest::set_body(std::string body) {
    body_ = std::move(body);
}

} // namespace project::core

