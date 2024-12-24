#pragma once

#include <boost/log/trivial.hpp>     
#include <boost/log/core.hpp>        
#include <boost/log/expressions.hpp> 
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
#include <unordered_map>
#include <chrono>

#define LOG_SERVER_START(port, address) \
    logger::Log({{"port"s, port}, {"address"s, address}}, logger::LogMessages::SERVER_STARTED);  

#define LOG_SERVER_EXIT(code, ...) \
    logger::Log({{"code"s, code} __VA_OPT__(, {"exception"s, __VA_ARGS__})}, logger::LogMessages::SERVER_EXITED); 

#define LOG_REQUEST_RECEIVED(ip, URI, method) \
    logger::Log({{"ip"s, ip}, {"URI"s, URI}, {"method", method}}, logger::LogMessages::REQUEST_RECEIVED);

#define LOG_RESPONSE_SENT(esponse_time, code, content_type) \
    logger::Log({{"response_time"s, response_time}, {"code"s, code}, {"content_type", content_type}}, logger::LogMessages::RESPONSE_SENT);

#define LOG_ERROR(code, text, where) \
    logger::Log({{"code"s, code}, {"text"s, text}, {"where", where}}, logger::LogMessages::ERROR);

#define LOG_TO_CONSOLE() logger::InitBoostLog();

namespace logger {
using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace json = boost::json;

enum class LogMessages{
    SERVER_STARTED,
    SERVER_EXITED,
    REQUEST_RECEIVED,
    RESPONSE_SENT,
    ERROR
};

class Timer{
public:
    void Start(){
        start_ = std::chrono::system_clock::now();
    }

    size_t End(){
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_).count();
        start_.min();
        return dur;
    }
private:
    std::chrono::system_clock::time_point start_ = std::chrono::system_clock::now();
};

static const std::unordered_map<LogMessages, std::string> StrMessages {
    {LogMessages::SERVER_STARTED, "server started"},
    {LogMessages::SERVER_EXITED, "server exited"},
    {LogMessages::REQUEST_RECEIVED, "request received"},
    {LogMessages::RESPONSE_SENT, "response sent"},
    {LogMessages::ERROR, "error"},
};

std::ostream& operator<<(std::ostream& out, LogMessages msg);

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
void InitBoostLog();

void Log(const json::value& data, LogMessages message);

}; // logger