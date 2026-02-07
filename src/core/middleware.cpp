#include "project/core/middleware.h"
#include "project/core/http_request.h"
#include "project/core/http_response.h"
#include "project/core/request_context.h"

namespace project::core {

void MiddlewareChain::add(MiddlewarePtr middleware) {
    chain_.push_back(std::move(middleware));
}

bool MiddlewareChain::execute(
    HttpRequest& request,
    HttpResponse& response,
    RequestContext& context
) const {
    for (const auto& middleware : chain_) {
        if (!middleware->handle(request, response, context)) {
            return false;
        }
    }
    return true;
}

} // namespace project::core

