#pragma once

#include "http_server.h"
#include "model.h"
#include "players.h"

#include <boost/json.hpp>
#include <string_view>
#include <map>
#include <filesystem>
#include <cassert>
#include <variant>

namespace http_handler
{

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace fs = std::filesystem;
    namespace sys = boost::system;
    namespace json = boost::json;
    namespace net = boost::asio;

    using namespace std::literals;

    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;
    using FileResponse = http::response<http::file_body>;
    using VariantResponse = std::variant<StringResponse, FileResponse>;
    using Strand = net::strand<net::io_context::executor_type>;

    class LogicHandler
    {
    public:
        static StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                                 bool keep_alive,
                                                 std::string_view content_type, std::string cache)
        {
            StringResponse response(status, http_version);
            if (cache == "no-cache")
                response.set(http::field::cache_control, cache);
            response.set(http::field::content_type, content_type);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);

            return response;
        }

        static FileResponse MakeFileResponse(http::status status, http::file_body::value_type &body, unsigned http_version,
                                             bool keep_alive,
                                             std::string_view content_type)
        {
            FileResponse response;

            response.version(http_version);
            response.result(status);
            response.insert(http::field::content_type, content_type);
            response.body() = std::move(body);
            response.prepare_payload();

            return response;
        }

        static StringResponse ReportServerError(http::status status, std::string_view code_message, unsigned http_version,
                                                bool keep_alive, std::string_view content_type, std::string_view cache = "",
                                                std::string_view allow_method = "")
        {
            StringResponse response(status, http_version);

            if (cache == "no-cache")
                response.set(http::field::cache_control, cache);
            if (allow_method == "POST")
                response.set(http::field::allow, allow_method);
            if (allow_method == "GET, HEAD")
                response.set(http::field::allow, allow_method);

            response.set(http::field::content_type, content_type);
            response.body() = code_message;
            response.content_length(code_message.size());
            response.keep_alive(keep_alive);

            return response;
        }

        struct ContentType
        {
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

        std::string URLDecode(const std::string &encoded);
        static bool StartWithStr(const std::string &decoded, const std::string &target);
        static bool IsSubPath(fs::path path, fs::path base);
        std::string ToLower(const std::string str);
        std::string_view GetContentType(const std::string &req_target);
        std::string StatusCodeProcessing(int code);
    };

    class HandlerApiRequest : public LogicHandler
    {
    public:
        explicit HandlerApiRequest(Strand api_strand)
            : strand_{api_strand}
        {
        }
        template <typename Request>
        StringResponse ApiHandleRequest(Request &&req, model::Game &game)
        {
            const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view content_type, std::string cache = "")
            {
                return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type, cache);
            };
            const auto file_response = [this, &req](http::status status, http::file_body::value_type &body, std::string_view content_type)
            {
                return MakeFileResponse(status, body, req.version(), req.keep_alive(), content_type);
            };
            const auto error_response = [this, &req](http::status status, std::string_view body, std::string_view content_type, std::string_view cache = "", std::string allow = "")
            {
                return ReportServerError(status, body, req.version(), req.keep_alive(), content_type, cache, allow);
            };

            try
            {
                auto req_method = req.method();
                auto version = req.version();
                auto kepp_alive = req.keep_alive();
                std::string decoded = LogicHandler::URLDecode(std::string(req.target()));

                if (StartWithStr(decoded, "/api/v1/maps"))
                {
                    std::pair<std::string, bool> request = MapRequest(decoded, game);
                    if (request.second == false)
                    {
                        return error_response(http::status::not_found, request.first, ContentType::JSON_HTML);
                    }
                    else
                    {
                        return text_response(http::status::ok, request.first, ContentType::JSON_HTML);
                    }
                }
                if (StartWithStr(decoded, "/api/v1/game/players"))
                {
                    try
                    {
                        auto it = req.find(http::field::authorization);
                        std::string_view req_token = it->value();
                        players::Token token(std::string(req_token.substr(7, req_token.npos)));

                        if (req.method() != http::verb::get && req.method() != http::verb::head)
                        {
                            json::object error_code;
                            error_code["code"] = "invalidMethod";
                            error_code["message"] = "Invalid method";
                            return error_response(http::status::method_not_allowed, json::serialize(error_code), ContentType::JSON_HTML, "no-cache", "GET, HEAD");
                        }
                        if ((*token).size() != 32)
                        {
                            json::object error_code;
                            error_code["code"] = "invalidToken";
                            error_code["message"] = "Authorization header is missing";
                            return error_response(http::status::unauthorized, json::serialize(error_code), ContentType::JSON_HTML, "no-cache");
                        }

                        if (players_.FindByToken(token))
                        {
                            std::string respons_body = GetPlayersInfo(players_.GetPlayers());
                            return text_response(http::status::ok, respons_body, ContentType::JSON_HTML, "no-cache");
                        }
                        json::object error_code;
                        error_code["code"] = "unknownToken";
                        error_code["message"] = "Player token has not been found";
                        return text_response(http::status::unauthorized, json::serialize(error_code), ContentType::JSON_HTML, "no-cache");
                    }
                    catch (std::exception &e)
                    {
                        json::object error_code;
                        error_code["code"] = "invalidToken";
                        error_code["message"] = "Authorization header is missing";
                        return text_response(http::status::unauthorized, json::serialize(error_code), ContentType::JSON_HTML, "no-cache");
                    }
                }
                if (StartWithStr(decoded, "/api/v1/game/join"))
                {
                    if (req.method() != http::verb::post)
                    {
                        json::object error_code;
                        error_code["code"] = "invalidMethod";
                        error_code["message"] = "Only POST method is expected";
                        return error_response(http::status::method_not_allowed, json::serialize(error_code), ContentType::JSON_HTML, "no-cache", "POST");
                    }
                    json::object player = json::parse(req.body()).as_object();
                    if (player.count("userName"s) && player.count("mapId"))
                    {
                        std::string name = std::string(player.at("userName").as_string());
                        std::string map_id = std::string(player.at("mapId").as_string());

                        if (name.empty())
                        {
                            json::object error_code;
                            error_code["code"] = "invalidArgument";
                            error_code["message"] = "Invalid name";
                            return error_response(http::status::bad_request, json::serialize(error_code), ContentType::JSON_HTML, "no-cache");
                        }

                        if (game.FindMap(model::Map::Id(map_id)) == nullptr)
                        {
                            std::string error_code = LogicHandler::StatusCodeProcessing(404);
                            return text_response(http::status::not_found, error_code, ContentType::JSON_HTML, "no-cache");
                        }

                        std::string respons_body = AuthorisationPlayer(name, map_id, game);
                        return text_response(http::status::ok, respons_body, ContentType::JSON_HTML, "no-cache");
                    }
                    else
                    {
                        json::object error_code;
                        error_code["code"] = "invalidArgument";
                        error_code["message"] = "Join game request parse error";
                        return error_response(http::status::bad_request, json::serialize(error_code), ContentType::JSON_HTML, "no-cache");
                    }
                }
                std::string respons_body = LogicHandler::StatusCodeProcessing(400);
                return text_response(http::status::bad_request, respons_body, ContentType::JSON_HTML);
            }
            catch (std::exception &e)
            {
                return error_response(http::status::internal_server_error, "internal_server_error", ContentType::JSON_HTML);
            }
        }

    private:
        friend class RequestHandler;
        Strand strand_;
        int random_id = 1;

        players::Players players_;

        std::pair<std::string, bool> MapRequest(const std::string &decoded, const model::Game &game);

        std::string AuthorisationPlayer(std::string &name, std::string &map_id, model::Game &game);

        std::string GetPlayersInfo(const players::Players::TokenPlayer &players);
    };

    class HandlerFIleRequest : public LogicHandler
    {
    public:
        HandlerFIleRequest(const std::string &root)
            : static_path_root_(fs::canonical(fs::path(root)))
        {
        }
        template <typename Request>
        VariantResponse FileHandleRequest(Request &&req)
        {
            if (req.method() != http::verb::get)
            {
                return ReportServerError(http::status::method_not_allowed, "Invalid method", req.version(), req.keep_alive(), ContentType::JSON_HTML);
            }

            std::string decoded = LogicHandler::URLDecode(std::string(req.target()));
            if (req.target().empty() || req.target() == "/")
            {
                std::string start_file = "index.html";
                if (decoded != "index file")
                {
                    start_file = URLDecode(std::string(req.target().substr(1)));
                }
                http::file_body::value_type file;
                std::string_view content_type = GetContentType(start_file);
                fs::path required_path(start_file);
                fs::path summary_path = fs::weakly_canonical(static_path_root_ / required_path);
                if (!IsSubPath(summary_path, static_path_root_))
                {
                    return ReportServerError(http::status::forbidden, "Access denied", req.version(), req.keep_alive(), ContentType::TEXT_PLAIN);
                }
                if (sys::error_code ec; file.open(summary_path.string().data(), beast::file_mode::read, ec), ec)
                {
                    return ReportServerError(http::status::not_found, "Need more learning", req.version(), req.keep_alive(), ContentType::TEXT_PLAIN);
                }
                return MakeFileResponse(http::status::ok, file, req.version(), req.keep_alive(), content_type);
            }
            return ReportServerError(http::status::not_found, "Need more learning", req.version(), req.keep_alive(), ContentType::TEXT_PLAIN); 
        }

    private:
        fs::path static_path_root_;
    };

    class RequestHandler : public std::enable_shared_from_this<RequestHandler>
    {
    public:
        explicit RequestHandler(model::Game& game, fs::path static_root, Strand api_strand)
            : game_(game), file_handler(static_root), api_handler{api_strand}
        {
        }

        RequestHandler(const RequestHandler &) = delete;
        RequestHandler &operator=(const RequestHandler &) = delete;

        template <typename Request, typename Send>
        void operator()(Request &&req, Send &&send)
        {
            std::string decoded = logic_handler.URLDecode(std::string(req.target()));
            if (logic_handler.StartWithStr(decoded, "/api/"))
            {
                auto handle = [self = shared_from_this(), send, req]
                {
                    try
                    {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_handler.strand_.running_in_this_thread());
                        return send(self->api_handler.ApiHandleRequest(req, self->game_));
                    }
                    catch (std::exception &ex)
                    {
                        send(self->logic_handler.ReportServerError(http::status::bad_request, "badRequest",
                                                                   req.version(), req.keep_alive(), "application/json"));
                    }
                };
                return net::dispatch(api_handler.strand_, handle);
            }

            /* Запросы доступа к файлам обрабатывает FileHandler*/
            return std::visit(
                [&send, &req](auto &&result)
                {
                    send(std::forward<decltype(result)>(result));
                },
                file_handler.FileHandleRequest(std::forward<decltype(req)>(req)));
        }

    private:
        model::Game& game_;
        LogicHandler logic_handler;
        HandlerApiRequest api_handler;
        HandlerFIleRequest file_handler;
    };

} // namespace http_handler
