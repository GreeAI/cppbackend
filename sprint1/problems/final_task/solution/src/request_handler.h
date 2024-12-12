#pragma once

#include "http_server.h"
#include "model.h"


#include <string_view>
#include <map>


namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

class RequestHandler {
public:

    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::shared_ptr<StringResponse> response;

        response = std::make_shared<StringResponse>(HandleRequest(std::move(req)));

        send(std::move(*response));
    }

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view JSON_HTML = "application/json"sv;
    };


    // Создаёт StringResponse с заданными параметрами
    static StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                    bool keep_alive,
                                    std::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    StringResponse HandleRequest(StringRequest&& req);

private:

    model::Game& game_;
};

}  // namespace http_handler
