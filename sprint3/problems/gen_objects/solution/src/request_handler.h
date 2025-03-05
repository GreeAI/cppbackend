#pragma once

#include <boost/beast/http/verb.hpp>
#include <boost/json.hpp>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <set>
#include <string_view>
#include <variant>

#include "app.h"
#include "http_server.h"
#include "model.h"
#include "request_struct.h"

namespace http_handler {

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
using Milliseconds = std::chrono::milliseconds;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
  using Strand = net::strand<net::io_context::executor_type>;
  using Handler = std::function<void(std::chrono::milliseconds delta)>;

  // Функция handler будет вызываться внутри strand с интервалом period
  Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
      : strand_{strand}, period_{period}, handler_{std::move(handler)} {}

  void Start() {
    net::dispatch(strand_, [self = shared_from_this()] {
      self->last_tick_ = Clock::now();
      self->ScheduleTick();
    });
  }

private:
  void ScheduleTick() {
    assert(strand_.running_in_this_thread());
    timer_.expires_after(period_);
    timer_.async_wait(
        [self = shared_from_this()](sys::error_code ec) { self->OnTick(ec); });
  }

  void OnTick(sys::error_code ec) {
    using namespace std::chrono;
    assert(strand_.running_in_this_thread());

    if (!ec) {
      auto this_tick = Clock::now();
      auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
      last_tick_ = this_tick;
      try {
        handler_(delta);
      } catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
      }
      ScheduleTick();
    }
  }

  using Clock = std::chrono::steady_clock;

  Strand strand_;
  std::chrono::milliseconds period_;
  net::steady_timer timer_{strand_};
  Handler handler_;
  std::chrono::steady_clock::time_point last_tick_;
};

// ------------------------------- LogicHandler -------------------------------------

class LogicHandler {
public:
  static StringResponse
  MakeStringResponse(http::status status, std::string_view body,
                     unsigned http_version, bool keep_alive,
                     std::string_view content_type, std::string cache) {
    StringResponse response(status, http_version);
    if (cache == "no-cache")
      response.set(http::field::cache_control, cache);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);

    return response;
  }

  static FileResponse MakeFileResponse(http::status status,
                                       http::file_body::value_type &body,
                                       unsigned http_version, bool keep_alive,
                                       std::string_view content_type) {
    FileResponse response;

    response.version(http_version);
    response.result(status);
    response.insert(http::field::content_type, content_type);
    response.body() = std::move(body);
    response.prepare_payload();

    return response;
  }

  static StringResponse
  ReportServerError(http::status status, std::string_view code_message,
                    unsigned http_version, bool keep_alive,
                    std::string_view content_type, std::string_view cache = "",
                    std::string_view allow_method = "") {
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

  std::string URLDecode(const std::string &encoded);
  static bool StartWithStr(const std::string &decoded,
                           const std::string &target);
  static bool IsSubPath(fs::path path, fs::path base);
  std::string ToLower(const std::string str);
  std::string_view GetContentType(const std::string &req_target);
  std::string StatusCodeProcessing(int code);

  template <typename... Args> void SetMethods(Args &&...args) {
    (methods_.insert(std::forward<Args>(args)), ...);
  }

  bool FindMethod(std::string &method) { return methods_.count(method); }

  Milliseconds FromDouble(double tick) {
    return std::chrono::duration_cast<Milliseconds>(
        std::chrono::duration<double>(tick / 1000));
  }

private:
  std::set<std::string> methods_;
};

// -----------------------HandleApiRequest---------------------------

class HandlerApiRequest : public LogicHandler {
public:
  explicit HandlerApiRequest(Strand api_strand, model::Game &game,
                             std::optional<double> tick, bool random_spawn)
      : strand_{api_strand}, app_(game, tick.has_value(), random_spawn),
        ticker_(), loot_ticker_() {
    app_.GenerateLoot(Milliseconds{0});
    if (tick.has_value()) {
      ticker_ = std::make_shared<Ticker>(
          strand_, FromDouble(*tick), [this](Milliseconds delta) {
            this->app_.TickTime(delta.count() / 1000);
          });
      ticker_->Start();

      loot_ticker_ = std::make_shared<Ticker>(
          strand_, game.GetLootGenerator(), [this](Milliseconds delta) {
              this->app_.GenerateLoot(delta);
          });

      loot_ticker_->Start();
    }
  }

private:
  friend class RequestHandler;
  Strand strand_;
  int random_id = 1;
  app::Aplication app_;

  std::shared_ptr<Ticker> ticker_;
  std::shared_ptr<Ticker> loot_ticker_;

  players::Players players_;

public:
  template <typename Request>
  StringResponse ApiHandleRequest(Request &&req, model::Game &game) {
    const auto text_response = [this, &req](http::status status,
                                            std::string_view text,
                                            std::string_view content_type,
                                            std::string cache = "no-cache") {
      return MakeStringResponse(status, text, req.version(), req.keep_alive(),
                                content_type, cache);
    };
    const auto file_response = [this, &req](http::status status,
                                            http::file_body::value_type &body,
                                            std::string_view content_type) {
      return MakeFileResponse(status, body, req.version(), req.keep_alive(),
                              content_type);
    };
    const auto error_response = [this, &req](
                                    http::status status, std::string_view body,
                                    std::string_view content_type,
                                    std::string_view cache = "no-cache",
                                    std::string allow = "") {
      return ReportServerError(status, body, req.version(), req.keep_alive(),
                               content_type, cache, allow);
    };

    try {
      std::string decoded = LogicHandler::URLDecode(std::string(req.target()));

      if (StartWithStr(decoded, "/api/v1/maps")) {
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head) {
          json::object error_code;
          error_code["code"] = "invalidMethod";
          error_code["message"] = "Invalid method";
          return error_response(
              http::status::method_not_allowed, json::serialize(error_code),
              ContentType::JSON_HTML, "no-cache", "GET, HEAD");
        }

        std::pair<std::string, bool> request = MapRequest(decoded, game);

        if (request.second == false) {
          return error_response(http::status::not_found, request.first,
                                ContentType::JSON_HTML);
        } else {
          return text_response(http::status::ok, request.first,
                               ContentType::JSON_HTML);
        }
      }
      if (StartWithStr(decoded, "/api/v1/game/players")) {
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head) {
          json::object error_code;
          error_code["code"] = "invalidMethod";
          error_code["message"] = "Invalid method";
          return error_response(
              http::status::method_not_allowed, json::serialize(error_code),
              ContentType::JSON_HTML, "no-cache", "GET, HEAD");
        }
        try {
          auto it = req.find(http::field::authorization);
          std::string_view req_token = it->value();
          players::Token token(
              std::string(req_token.substr(7, req_token.npos)));

          if ((*token).size() != 32) {
            json::object error_code;
            error_code["code"] = "invalidToken";
            error_code["message"] = "Authorization header is missing";
            return error_response(http::status::unauthorized,
                                  json::serialize(error_code),
                                  ContentType::JSON_HTML, "no-cache");
          }

          if (app_.FindByToken(token)) {
            std::string respons_body = app_.GetPlayersInfo();
            return text_response(http::status::ok, respons_body,
                                 ContentType::JSON_HTML, "no-cache");
          }
          json::object error_code;
          error_code["code"] = "unknownToken";
          error_code["message"] = "Player token has not been found";
          return text_response(http::status::unauthorized,
                               json::serialize(error_code),
                               ContentType::JSON_HTML, "no-cache");
        } catch (std::exception &e) {
          json::object error_code;
          error_code["code"] = "invalidToken";
          error_code["message"] = "Authorization header is missing";
          return text_response(http::status::unauthorized,
                               json::serialize(error_code),
                               ContentType::JSON_HTML, "no-cache");
        }
      }
      if (StartWithStr(decoded, "/api/v1/game/join")) {
        if (req.method() != http::verb::post) {
          json::object error_code;
          error_code["code"] = "invalidMethod";
          error_code["message"] = "Only POST method is expected";
          return error_response(http::status::method_not_allowed,
                                json::serialize(error_code),
                                ContentType::JSON_HTML, "no-cache", "POST");
        }
        json::object player = json::parse(req.body()).as_object();
        if (player.count("userName"s) && player.count("mapId")) {
          std::string name = std::string(player.at("userName").as_string());
          std::string map_id = std::string(player.at("mapId").as_string());
          try {
            std::string response_body = app_.JoinGame(map_id, name);
            return text_response(http::status::ok, response_body,
                                 ContentType::JSON_HTML, "no-cache");
          } catch (const app::JoinGameError &error_code) {
            auto error = error_code.GetError();
            if (error.first == "bad_request") {
              return error_response(http::status::bad_request, error.second,
                                    ContentType::JSON_HTML, "no-cache");
            }
            return error_response(http::status::bad_request, error.second,
                                  ContentType::JSON_HTML, "no-cache");
          }
        } else {
          json::object error_code;
          error_code["code"] = "invalidArgument";
          error_code["message"] = "Join game request parse error";
          return error_response(http::status::bad_request,
                                json::serialize(error_code),
                                ContentType::JSON_HTML, "no-cache");
        }
      }
      if (StartWithStr(decoded, "/api/v1/game/state")) {
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head) {
          json::object error_code;
          error_code["code"] = "invalidMethod";
          error_code["message"] = "Invalid method";
          return error_response(
              http::status::method_not_allowed, json::serialize(error_code),
              ContentType::JSON_HTML, "no-cache", "GET, HEAD");
        }
        try {
          auto it = req.find(http::field::authorization);
          std::string_view req_token = it->value();
          players::Token token(
              std::string(req_token.substr(7, req_token.npos)));

          if ((*token).size() != 32) {
            json::object error_code;
            error_code["code"] = "invalidToken";
            error_code["message"] = "Authorization header is missing";
            return error_response(http::status::unauthorized,
                                  json::serialize(error_code),
                                  ContentType::JSON_HTML, "no-cache");
          }

          if (app_.FindByToken(token)) {
            std::string respons_body = app_.GetGameState(token);
            return text_response(http::status::ok, respons_body,
                                 ContentType::JSON_HTML, "no-cache");
          }
          json::object error_code;
          error_code["code"] = "unknownToken";
          error_code["message"] = "Player token has not been found";
          return text_response(http::status::unauthorized,
                               json::serialize(error_code),
                               ContentType::JSON_HTML, "no-cache");
        } catch (std::exception &e) {
          json::object error_code;
          error_code["code"] = "invalidToken";
          error_code["message"] = "Authorization header is missing";
          return text_response(http::status::unauthorized,
                               json::serialize(error_code),
                               ContentType::JSON_HTML, "no-cache");
        }
      }
      if (StartWithStr(decoded, "/api/v1/game/player/action")) {
        if (req.method() != http::verb::post) {
          json::object error_code;
          error_code["code"] = "invalidMethod";
          error_code["message"] = "Only POST method is expected";
          return error_response(http::status::method_not_allowed,
                                json::serialize(error_code),
                                ContentType::JSON_HTML, "no-cache", "POST");
        }
        try {
          auto it = req.find(http::field::authorization);
          std::string_view req_token = it->value();
          players::Token token(
              std::string(req_token.substr(7, req_token.npos)));

          if ((*token).size() != 32) {
            json::object error_code;
            error_code["code"] = "invalidToken";
            error_code["message"] = "Authorization header is missing";
            return error_response(http::status::unauthorized,
                                  json::serialize(error_code),
                                  ContentType::JSON_HTML, "no-cache");
          }
          auto move_req = json::parse(req.body()).as_object();
          if (!move_req.contains("move")) {
            json::object error_code;
            error_code["code"] = "invalidArgument";
            error_code["message"] = "Failed to parse action";
            return error_response(http::status::bad_request,
                                  json::serialize(error_code),
                                  ContentType::JSON_HTML, "no-cache");
          }
          std::string move_dir = std::string(move_req.at("move").as_string());

          if (auto player = app_.FindByToken(token)) {
            std::string respons_body = app_.PlayerAction(player, move_dir);
            return text_response(http::status::ok, respons_body,
                                 ContentType::JSON_HTML, "no-cache");
          }
          json::object error_code;
          error_code["code"] = "unknownToken";
          error_code["message"] = "Player token has not been found";
          return text_response(http::status::unauthorized,
                               json::serialize(error_code),
                               ContentType::JSON_HTML, "no-cache");
        } catch (std::exception &e) {
          json::object error_code;
          error_code["code"] = "invalidToken";
          error_code["message"] = "Authorization header is missing";
          return text_response(http::status::unauthorized,
                               json::serialize(error_code),
                               ContentType::JSON_HTML, "no-cache");
        }
      }
      if (StartWithStr(decoded, "/api/v1/game/tick")) {
        if (req.method() != http::verb::post) {
          json::object error_code;
          error_code["code"] = "invalidMethod";
          error_code["message"] = "Only POST method is expected";
          return error_response(http::status::method_not_allowed,
                                json::serialize(error_code),
                                ContentType::JSON_HTML, "no-cache", "POST");
        }
        if (app_.IsTickSet()) {
          json::object error_code;
          error_code["code"] = "invalidArgument";
          error_code["message"] = "Invalid endpoint";
          return error_response(http::status::bad_request,
                                json::serialize(error_code),
                                ContentType::JSON_HTML, "no-cache");
        }

        try {
          json::object tick_time = json::parse(req.body()).as_object();
          if (tick_time.count("timeDelta")) {
            double tick =
                static_cast<double>(tick_time.at("timeDelta").as_int64()) /
                1000;
            std::string respons_body = app_.TickTime(tick);
            return text_response(http::status::ok, respons_body,
                                 ContentType::JSON_HTML, "no-cache");
          }
        } catch (std::exception &e) {
          json::object error_code;
          error_code["code"] = "invalidArgument";
          error_code["message"] = "Failed to parse tick request JSON";
          return error_response(http::status::bad_request,
                                json::serialize(error_code),
                                ContentType::JSON_HTML, "no-cache");
        }
      }
      std::string respons_body = LogicHandler::StatusCodeProcessing(400);
      return text_response(http::status::bad_request, respons_body,
                           ContentType::JSON_HTML);
    } catch (std::exception &e) {
      return error_response(http::status::internal_server_error,
                            "internal_server_error", ContentType::JSON_HTML);
    }
  }

private:
  std::pair<std::string, bool> MapRequest(const std::string &decoded,
                                          const model::Game &game);
};

class HandlerFIleRequest : public LogicHandler {
public:
  HandlerFIleRequest(const fs::path &root)
      : static_path_root_(fs::canonical(root)) {}
  template <typename Request> VariantResponse FileHandleRequest(Request &&req) {
    if (req.method() != http::verb::get) {
      return ReportServerError(http::status::method_not_allowed,
                               "Invalid method", req.version(),
                               req.keep_alive(), ContentType::JSON_HTML);
    }

    std::string decoded = LogicHandler::URLDecode(std::string(req.target()));
    if (decoded.empty() || decoded == "/") {
      decoded = "empty";
    }
    if (!decoded.empty()) {
      std::string start_file = "index.html";
      if (decoded != "empty") {
        start_file =
            LogicHandler::URLDecode(std::string(req.target().substr(1)));
      }
      http::file_body::value_type file;
      std::string_view content_type = GetContentType(start_file);
      fs::path required_path(start_file);
      fs::path summary_path =
          fs::weakly_canonical(static_path_root_ / required_path);
      if (!IsSubPath(summary_path, static_path_root_)) {
        return ReportServerError(http::status::forbidden, "Access denied",
                                 req.version(), req.keep_alive(),
                                 ContentType::TEXT_PLAIN);
      }
      if (sys::error_code ec;
          file.open(summary_path.string().data(), beast::file_mode::read, ec),
          ec) {
        return ReportServerError(http::status::not_found, "Need more learning",
                                 req.version(), req.keep_alive(),
                                 ContentType::TEXT_PLAIN);
      }
      return MakeFileResponse(http::status::ok, file, req.version(),
                              req.keep_alive(), content_type);
    }
    return ReportServerError(http::status::not_found, "After if block",
                             req.version(), req.keep_alive(),
                             ContentType::TEXT_PLAIN);
  }

private:
  fs::path static_path_root_;
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
  explicit RequestHandler(model::Game &game, strct::Args &args,
                          Strand api_strand)
      : game_(game), file_handler(args.root),
        api_handler{api_strand, game, args.tick, args.random_spawn} {}

  RequestHandler(const RequestHandler &) = delete;
  RequestHandler &operator=(const RequestHandler &) = delete;

  template <typename Request, typename Send>
  void operator()(Request &&req, Send &&send) {
    std::string decoded = logic_handler.URLDecode(std::string(req.target()));
    if (logic_handler.StartWithStr(decoded, "/api/")) {
      auto handle = [self = shared_from_this(), send, req] {
        try {
          // Этот assert не выстрелит, так как лямбда-функция будет выполняться
          // внутри strand
          assert(self->api_handler.strand_.running_in_this_thread());
          return send(self->api_handler.ApiHandleRequest(req, self->game_));
        } catch (std::exception &ex) {
          send(self->logic_handler.ReportServerError(
              http::status::bad_request, "badRequest", req.version(),
              req.keep_alive(), "application/json"));
        }
      };
      return net::dispatch(api_handler.strand_, handle);
    }

    /* Запросы доступа к файлам обрабатывает FileHandler*/
    return std::visit(
        [&send, &req](auto &&result) {
          send(std::forward<decltype(result)>(result));
        },
        file_handler.FileHandleRequest(std::forward<decltype(req)>(req)));
  }

private:
  model::Game &game_;
  LogicHandler logic_handler;
  HandlerApiRequest api_handler;
  HandlerFIleRequest file_handler;
};

} // namespace http_handler
