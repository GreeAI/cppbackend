#include "request_handler.h"
#include "json_loader.h"

#include <fstream>

namespace http_handler {
    //Разбить на отдельные методы, добавить валидацию путя. Мои глаза...
    RequestHandler::VariantResponse RequestHandler::HandleRequest(StringRequest&& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view content_type) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
        };
        const auto file_response = [this, &req](http::status status, http::file_body::value_type& body, std::string_view content_type) {
            return MakeFileResponse(status, body, req.version(), req.keep_alive(), content_type);
        };
        try{
            if(req.method() == http::verb::get){

                std::string decoded = URLDecode(std::string(req.target()));

                if(decoded.empty() || decoded == "/") {
                    decoded = "index.html";
                }
                std::cout << "decoded: " << decoded << std::endl;

                if(decoded == "/api/v1/maps") {
                    const model::Game::Maps maps = game_.GetMaps();
                    const std::string respons_body = json_loader::MapIdName(maps);
                    return text_response(http::status::ok, respons_body, ContentType::JSON_HTML);
                } 
                else if (StartWithStr(decoded ,"/api/v1/maps/")) {
                    std::string map_id = decoded.substr(std::string_view("/api/v1/maps/").length());
                    const model::Game::Maps& maps = game_.GetMaps();
                    const std::string respons_body = json_loader::MapFullInfo(maps);
                    if(respons_body.find(map_id) != std::string::npos) {
                        return text_response(http::status::ok, respons_body, ContentType::JSON_HTML);
                    }
                    int status_code = 404;
                    std::string error_code = json_loader::StatusCodeProcessing(status_code);
                    return text_response(http::status::not_found, error_code, ContentType::JSON_HTML);
                }
                else if (StartWithStr(decoded, "/api")) {
                    int status_code = 400;
                    std::string respons_body = json_loader::StatusCodeProcessing(status_code);
                    return text_response(http::status::bad_request, respons_body, ContentType::JSON_HTML); 
                }
                else {
                    std::string decoded_file = "index.html";
                    if(decoded != "index.html") {
                        decoded_file = URLDecode(std::string(req.target().substr(1)));
                    }
                    std::cout << "req.target(): " << req.target() << std::endl;
                    std::cout << "decoded_file: " << decoded_file << std::endl;
                    http::file_body::value_type file;
                    std::string_view content_type = GetContentType(decoded_file);
                    fs::path required_path(decoded_file);
                    fs::path summary_path = fs::weakly_canonical(static_path_root_ / required_path);
                    if (!IsSubPath(summary_path, static_path_root_)) {
                        return text_response(http::status::forbidden, "Access denied", ContentType::TEXT_PLAIN);
                    }
                    if (sys::error_code ec; file.open(summary_path.string().data(), beast::file_mode::read, ec), ec) {
                        return text_response(http::status::not_found, decoded_file, ContentType::TEXT_PLAIN);
                    }
                    return file_response(http::status::ok, file, content_type);
                }
            }
            return text_response(http::status::method_not_allowed, "Invalid method", ContentType::JSON_HTML);
        }
        catch(const std::exception& e) {
            return text_response(http::status::internal_server_error, "internal_server_error", ContentType::JSON_HTML);
        }
    }

std::string RequestHandler::URLDecode(const std::string& encoded) {
    std::string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            int value = std::stoi(hex, nullptr, 16);
            decoded += static_cast<char>(value);
            i += 2;
        } 
        else if (encoded[i] == '+') {
            decoded += ' ';
        } 
        else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

bool RequestHandler::StartWithStr(const std::string& decoded, const std::string& target) {
    if (target.size() > decoded.size()) return false;
    return decoded.compare(0, target.size(), target) == 0;
}

// Возвращает true, если каталог p содержится внутри base_path.
bool RequestHandler::IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}


std::string RequestHandler::ToLower(const std::string str){
    std::string result;
    for(char c : str){
        result.push_back(std::tolower(c));
    }

    return result;
}

std::string_view RequestHandler::GetContentType(const std::string& req_target) {
        size_t point = req_target.find_last_of('.');
        std::string file_extension = ToLower(std::string(req_target.substr(point + 1, req_target.npos)));

        if (file_extension == "html" || file_extension == "htm") {
            return RequestHandler::ContentType::TEXT_HTML;
        }
        else if (file_extension == "js") {
            return RequestHandler::ContentType::TEXT_JS;
        }
        else if (file_extension == "txt") {
            return RequestHandler::ContentType::TEXT_PLAIN;
        }
        else if (file_extension == "json") {
            return RequestHandler::ContentType::JSON_HTML;
        }
        else if (file_extension == "jpeg" || file_extension == "jpg" || file_extension == "jpe") {
            return RequestHandler::ContentType::IMAGE_JPEG;
        }
        else if (file_extension == "svg" || file_extension == "svgz") {
            return RequestHandler::ContentType::IMAGE_SVG;
        }
        else if (file_extension == "xml") {
            return RequestHandler::ContentType::APP_XML;
        }
        else if (file_extension == "css") {
            return RequestHandler::ContentType::TEXT_CSS;
        }
        else {
            return RequestHandler::ContentType::EMPTY;
        }
    }

}  // namespace http_handler

