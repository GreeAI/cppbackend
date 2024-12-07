#pragma once

#include "http_server.h"
#include "model.h"
#include "json_loader.h"


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
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                    bool keep_alive,
                                    std::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    StringResponse HandleRequest(StringRequest&& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view content_type) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
        };
        try{
            if(req.method() == http::verb::get){
                if(req.target() == "/api/v1/maps") {
                    const model::Game::Maps maps = game_.GetMaps();
                    const std::string respons_body = json_loader::MapIdName(maps);
                    return text_response(http::status::ok, respons_body, ContentType::JSON_HTML);
                } 
                else if (req.target().starts_with("/api/v1/maps/")) {
                    std::string_view map_id = req.target().substr(std::string_view("/api/v1/maps/").length());
                    const model::Game::Maps maps = game_.GetMaps();
                    const std::string respons_body = json_loader::MapFullInfo(maps);
                    if(respons_body.find(map_id) != std::string::npos) {
                        return text_response(http::status::ok, respons_body, ContentType::JSON_HTML);
                    }
                    int status_code = 404;
                    std::string error_code = json_loader::StatusCodeProcessing(status_code);
                    return text_response(http::status::not_found, error_code, ContentType::JSON_HTML);
                } 
                else {
                    int status_code = 400;
                    std::string respons_body = json_loader::StatusCodeProcessing(status_code);
                    return text_response(http::status::bad_request, respons_body, ContentType::JSON_HTML);
                }
            }
            return text_response(http::status::method_not_allowed, "Invalid method", ContentType::JSON_HTML);
        }
        catch(const std::exception& e) {
            return text_response(http::status::internal_server_error, "internal_server_error", ContentType::JSON_HTML);
        }
    }
private:

    model::Game& game_;
};

}  // namespace http_handler
