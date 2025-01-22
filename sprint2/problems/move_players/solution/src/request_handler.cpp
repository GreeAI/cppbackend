#include "request_handler.h"
#include "json_loader.h"

#include <fstream>

namespace http_handler
{
    using CT = LogicHandler::ContentType;
    /* ======================================= LogicHandler ======================================= */
    // URL-декодератор
    std::string LogicHandler::URLDecode(const std::string &encoded)
    {
        std::string decoded;
        for (size_t i = 0; i < encoded.length(); ++i)
        {
            if (encoded[i] == '%' && i + 2 < encoded.length())
            {
                std::string hex = encoded.substr(i + 1, 2);
                int value = std::stoi(hex, nullptr, 16);
                decoded += static_cast<char>(value);
                i += 2;
            }
            else if (encoded[i] == '+')
            {
                decoded += ' ';
            }
            else
            {
                decoded += encoded[i];
            }
        }
        return decoded;
    }

    // Аналог start_with для std::string
    bool LogicHandler::StartWithStr(const std::string &decoded, const std::string &target)
    {
        if (target.size() > decoded.size())
            return false;
        return decoded.compare(0, target.size(), target) == 0;
    }

    // Возвращает true, если каталог p содержится внутри base_path.
    bool LogicHandler::IsSubPath(fs::path path, fs::path base)
    {
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p)
        {
            if (p == path.end() || *p != *b)
            {
                return false;
            }
        }
        return true;
    }

    // Строку в строчные буквы, для GetContentType
    std::string LogicHandler::ToLower(const std::string str)
    {
        std::string result;
        for (char c : str)
        {
            result.push_back(std::tolower(c));
        }

        return result;
    }

    // Находит тип выводимых данных в content-type по расширению
    std::string_view LogicHandler::GetContentType(const std::string &req_target)
    {
        size_t point = req_target.find_last_of('.');
        std::string file_extension = ToLower(std::string(req_target.substr(point + 1, req_target.npos)));

        if (file_extension == "html" || file_extension == "htm")
        {
            return CT::TEXT_HTML;
        }
        else if (file_extension == "js")
        {
            return CT::TEXT_JS;
        }
        else if (file_extension == "txt")
        {
            return CT::TEXT_PLAIN;
        }
        else if (file_extension == "json")
        {
            return CT::JSON_HTML;
        }
        else if (file_extension == "jpeg" || file_extension == "jpg" || file_extension == "jpe")
        {
            return CT::IMAGE_JPEG;
        }
        else if (file_extension == "svg" || file_extension == "svgz")
        {
            return CT::IMAGE_SVG;
        }
        else if (file_extension == "xml")
        {
            return CT::APP_XML;
        }
        else if (file_extension == "css")
        {
            return CT::TEXT_CSS;
        }
        else
        {
            return CT::EMPTY;
        }
    }

    std::string LogicHandler::StatusCodeProcessing(int code)
    {
        json::object error_code;

        if (code == 400)
        {
            error_code["code"] = "badRequest";
            error_code["message"] = "Bad request";
        }
        else if (code == 404)
        {
            error_code["code"] = "mapNotFound";
            error_code["message"] = "Map not found";
        }
        return json::serialize(error_code);
    }
    /* ======================================= HandleApiRequest ======================================= */
    //Придумать как это всё разбить на разные методы

    //Jбработка запроса для .../maps...
    std::pair<std::string, bool> HandlerApiRequest::MapRequest(const std::string &decoded, const model::Game &game)
    {
        if (decoded == "/api/v1/maps")
        {
            std::string respons_body = json_loader::MapIdName(game.GetMaps());
            return std::make_pair(respons_body, true);
        }
        else if (StartWithStr(decoded, "/api/v1/maps/"))
        {
            model::Map::Id map_id(decoded.substr(std::string("/api/v1/maps/").length()));
            const model::Map* map = game.FindMap(map_id);
            if(map != nullptr) {
                std::string respons_body = json_loader::MapFullInfo(*map);
                return std::make_pair(respons_body, true);
            }
            std::string error_code = LogicHandler::StatusCodeProcessing(404);
            return std::make_pair(error_code, false);
        }
        std::string error_code = LogicHandler::StatusCodeProcessing(400);
        return std::make_pair(error_code, false);
    }

    /* ======================================= HandleFileRequest ======================================= */
} // namespace http_handler
