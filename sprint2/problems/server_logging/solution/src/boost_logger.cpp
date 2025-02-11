#include "boost_logger.h"

namespace logger {
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

std::ostream &operator<<(std::ostream &out, LogMessages msg) {
  out << StrMessages.at(msg);
  return out;
}

void MyFormatter(logging::record_view const &rec,
                 logging::formatting_ostream &strm) {
  auto ts = *rec[timestamp];
  strm << json::serialize(json::object{
      {"timestamp", to_iso_extended_string(*rec[timestamp])},
      {"data", *rec[additional_data]},
      {"message", json::string{*rec[logging::expressions::smessage]}}});
}

void InitBoostLog() {
  logging::add_common_attributes();

  logging::add_console_log(std::clog, keywords::format = &MyFormatter,
                           keywords::auto_flush = true);
}

void Log(const json::value &data, LogMessages message) {
  BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                          << message;
}

void LogServerStart(int port, const std::string &address) {
  Log({{"port"s, port}, {"address"s, address}}, LogMessages::SERVER_STARTED);
}

void LogServerExit(int code, const std::string &exception) {
  if (exception.empty()) {
    Log({{"code"s, code}}, LogMessages::SERVER_EXITED);
  } else {
    Log({{"code"s, code}, {"exception"s, exception}},
        LogMessages::SERVER_EXITED);
  }
}

void LogRequestReceived(const std::string &ip, const std::string &URI,
                        const std::string &method) {
  Log({{"ip"s, ip}, {"URI"s, URI}, {"method"s, method}},
      LogMessages::REQUEST_RECEIVED);
}

void LogResponseSent(int response_time, int code,
                     const std::string &content_type) {
  Log({{"response_time"s, response_time}
    , {"code"s, code}
    , {"content_type"s, content_type}}
    , LogMessages::RESPONSE_SENT);
}

void LogError(int code, const std::string &text, const std::string &where) {
  Log({{"code"s, code}, {"text"s, text}, {"where"s, where}},
      LogMessages::ERROR);
}
}; // namespace logger