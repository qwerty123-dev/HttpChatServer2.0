#include "project/core/request_context.h"

namespace project::core {

const std::string& RequestContext::request_id() const noexcept {
    return request_id_;
}

void RequestContext::set_request_id(std::string id) {
    request_id_ = std::move(id);
}

void RequestContext::set_user_id(std::string user_id) {
    user_id_ = std::move(user_id);
}

std::optional<std::string> RequestContext::user_id() const {
    return user_id_;
}

} // namespace project::core

