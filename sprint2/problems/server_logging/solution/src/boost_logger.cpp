#include "boost_logger.h"
#include <filesystem>
#include <sstream>

namespace logger{

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

std::ostream& operator<<(std::ostream& out, LogMessages msg){
    out << StrMessages.at(msg);
    return out;
}

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    strm << rec[additional_data] << ": ";

    auto ts = *rec[timestamp];
    strm << json::serialize(json::object{
        {"timestamp", to_iso_extended_string(*rec[timestamp])},
        {"data", *rec[additional_data]},
        {"message", json::string{*rec[logging::expressions::smessage]}}
    });
}

void InitBoostLog() {
    logging::add_common_attributes();
    std::filesystem::path file_path = std::filesystem::weakly_canonical( std::filesystem::path("../../logs/") / std::filesystem::path ("sample_%N.json"));

    logging::add_file_log(
        keywords::file_name = file_path.string(),
        keywords::format = &MyFormatter,
        keywords::open_mode = std::ios_base::app | std::ios_base::out,
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = logging::sinks::file::rotation_at_time_point(12, 0, 0)
    );

    logging::add_console_log( 
        std::clog,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

void Log(LogMessages message, const json::value& data) {
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << StrMessages.at(message);
}

void LogServerStarted(int port, const std::string& address) {
    json::object data;

    data["port"] = port;
    data["address"] = address;

    Log(LogMessages::SERVER_STARTED, data);

}

void LogServerExited(int code, const std::string& exception) {
    json::object data;

    data["code"] = code;

    if (!exception.empty()) {
        data["exception"] = exception;
    }

    Log(LogMessages::SERVER_EXITED, data);

}

void LogRequestReceived(const std::string& ip, const std::string& uri, const std::string& method) {
    json::object data;

    data["ip"] = ip;
    data["URI"] = uri;
    data["method"] = method;

    Log(LogMessages::REQUEST_RECEIVED, data);

}

void LogResponseSent(int response_time, int code, const std::string& content_type) {
    json::object data;

    data["response_time"] = response_time;
    data["code"] = code;
    data["content_type"] = content_type.empty() ? nullptr : content_type;

    Log(LogMessages::RESPONSE_SENT, data);

}

void LogError(int code, const std::string& text, const std::string& where) {
    json::object data;

    data["code"] = code;
    data["text"] = text;
    data["where"] = where;

    Log(LogMessages::ERROR, data);

}
}; // logger