#pragma once

#include "http_server.h"
#include "model.h"


#include <string_view>
#include <map>
#include <filesystem>
#include <cassert>
#include <variant>

namespace http_handler {
    
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace sys = boost::system;

using namespace std::literals;

class RequestHandler {
public:

    explicit RequestHandler(model::Game& game, const std::string& static_root)
        : game_{game}
        , static_path_root_{fs::canonical(fs::path(static_root))} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;
    using FileResponse = http::response<http::file_body>;
    using VariantResponse = std::variant<StringResponse, FileResponse>;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::visit(
                [&send, &req](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                HandleRequest(std::forward<decltype(req)>(req)));
    }

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_JS = "text/javascript"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view TEXT_CSS = "text/css"sv;
        constexpr static std::string_view JSON_HTML = "application/json"sv;
        constexpr static std::string_view APP_XML = "application/xml"sv;
        constexpr static std::string_view IMAGE_JPEG = "image/jpeg"sv;
        constexpr static std::string_view IMAGE_SVG = "image/svg+xml"sv;
        constexpr static std::string_view EMPTY = "application/octet-stream"sv;
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

    FileResponse MakeFileResponse(http::status status, http::file_body::value_type& body, unsigned http_version,
                                    bool keep_alive,
                                    std::string_view content_type) {
        FileResponse response;
        response.version(http_version);
        response.result(status);
        response.insert(http::field::content_type, content_type);
        response.body() = std::move(body);
        response.prepare_payload();
        return response;
    }

    VariantResponse HandleRequest(StringRequest&& req);
    std::string URLDecode(const std::string& encoded);
    bool StartWithStr(const std::string& decoded, const std::string& target);

    bool IsSubPath(fs::path path, fs::path base);
    std::string ToLower(const std::string str);
    std::string_view GetContentType(const std::string& req_target);


private:

    model::Game& game_;
    fs::path static_path_root_;
};

}  // namespace http_handler
