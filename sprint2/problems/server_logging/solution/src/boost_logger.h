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

namespace logger {
using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace json = boost::json;

enum class LogMessages{
    SERVER_STARTED,
    SERVER_EXITED,
    REQUEST_RECEIVED,
    RESPONSE_SENT,
    ERROR
};

static const std::unordered_map<LogMessages, std::string> StrMessages {
    {LogMessages::SERVER_STARTED, "server started"},
    {LogMessages::SERVER_EXITED, "server exited"},
    {LogMessages::REQUEST_RECEIVED, "request received"},
    {LogMessages::RESPONSE_SENT, "response sent"},
    {LogMessages::ERROR, "error"},
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

std::ostream& operator<<(std::ostream& out, LogMessages msg);

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
void InitBoostLog();

void Log(LogMessages message, const json::value& data);
void LogServerStarted(int port, const std::string& address);
void LogServerExited(int code, const std::string& exception = "");
void LogRequestReceived(const std::string& ip, const std::string& uri, const std::string& method);
void LogResponseSent(int response_time, int code, const std::string& content_type);
void LogError(int code, const std::string& text, const std::string& where);

}; // logger