#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace project::core {

class HttpRequest;
class HttpResponse;
class RequestContext;

class Middleware {
public:
    virtual ~Middleware() = default;

    // return false → остановить цепочку
    virtual bool handle(
        HttpRequest& request,
        HttpResponse& response,
        RequestContext& context
    ) = 0;
};

using MiddlewarePtr = std::shared_ptr<Middleware>;

class MiddlewareChain {
public:
    void add(MiddlewarePtr middleware);

    bool execute(
        HttpRequest& request,
        HttpResponse& response,
        RequestContext& context
    ) const;

private:
    std::vector<MiddlewarePtr> chain_;
};

} // namespace project::core

