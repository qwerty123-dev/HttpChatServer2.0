#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace project::core {

class HttpRequest;
class HttpResponse;
class RequestContext;

class HttpRouter {
public:
    using Handler = std::function<void(
        HttpRequest&,
        HttpResponse&,
        RequestContext&
    )>;

    HttpRouter() = default;

    void add_route(
        const std::string& method,
        const std::string& path,
        Handler handler
    );

    bool route(
        const std::string& method,
        const std::string& path,
        HttpRequest& request,
        HttpResponse& response,
        RequestContext& context
    ) const;

private:
    using MethodMap = std::unordered_map<std::string, Handler>;
    std::unordered_map<std::string, MethodMap> routes_;
};

} // namespace project::core

