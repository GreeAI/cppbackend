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

    VariantResponse MakeFileResponse(StringRequest& req) {
        FileResponse response;

        std::string_view content_type = GetContentType(req.target());
        std::string decoded = URLDecode(req.target().substr(1));

        http::file_body::value_type file;
        fs::path required_path(decoded);

        fs::path summary_path = fs::weakly_canonical(static_path_root_ / required_path);

        if (!IsSubPath(summary_path, static_path_root_)) {
            return MakeStringResponse(http::status::forbidden, "Access denied", req.version(), req.keep_alive(), ContentType::TEXT_PLAIN);
        }

        if (sys::error_code ec; file.open(summary_path.string().data(), beast::file_mode::read, ec), ec) {
            return MakeStringResponse(http::status::not_found, decoded, req.version(), req.keep_alive(), ContentType::TEXT_PLAIN);
        }

        response.version(req.version());
        response.result(http::status::ok);
        response.insert(http::field::content_type, content_type);
        response.body() = std::move(file);
        response.prepare_payload();
        return response;
    }

    VariantResponse HandleRequest(StringRequest&& req);
    char FromHexToChar(char a, char b);
    std::string URLDecode(const std::string_view encoded);
    bool StartWithStr(const std::string& decoded, const std::string& target);

    bool IsSubPath(fs::path path, fs::path base);
    std::string ToLower(const std::string str);
    std::string_view GetContentType(std::string req_target);


private:

    model::Game& game_;
    fs::path static_path_root_;
};

}  // namespace http_handler
