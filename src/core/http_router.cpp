#include "project/core/http_router.h"

namespace project::core {

void HttpRouter::add_route(
    const std::string& method,
    const std::string& path,
    Handler handler
) {
    routes_[path][method] = std::move(handler);
}

bool HttpRouter::route(
    const std::string& method,
    const std::string& path,
    HttpRequest& request,
    HttpResponse& response,
    RequestContext& context
) const {
    const auto path_it = routes_.find(path);
    if (path_it == routes_.end()) {
        return false;
    }

    const auto& method_map = path_it->second;
    const auto method_it = method_map.find(method);
    if (method_it == method_map.end()) {
        return false;
    }

    method_it->second(request, response, context);
    return true;
}

} // namespace project::core

