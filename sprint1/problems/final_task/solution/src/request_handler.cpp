#include "request_handler.h"
#include "json_loader.h"

namespace http_handler {

    RequestHandler::StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
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
                    const model::Game::Maps& maps = game_.GetMaps();
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
}  // namespace http_handler
