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
    /*std::filesystem::path file_path = std::filesystem::weakly_canonical( std::filesystem::path("../../logs/") / std::filesystem::path ("sample_%N.json"));

    logging::add_file_log(
        keywords::file_name = file_path.string(),
        keywords::format = &MyFormatter,
        keywords::open_mode = std::ios_base::app | std::ios_base::out,
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = logging::sinks::file::rotation_at_time_point(12, 0, 0)
    );*/

    logging::add_console_log( 
        std::clog,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

void Log(const json::value& data, LogMessages message) {
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << message;
}

}; // logger