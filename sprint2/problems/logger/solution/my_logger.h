#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <syncstream>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        auto tc = std::chrono::system_clock::to_time_t(GetTime());
        std::tm local_time; 
        std::ostringstream log;
        log << "/var/log/sample_log_" << std::put_time(std::localtime(&tc), "%Y_%m_%d") << ".log";
        std::string filename = log.str();
        return filename;
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::osyncstream logs(log_file_);
        logs << GetTimeStamp() << ": ";
        ((logs << args), ...);
        logs << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
        
        if(std::string new_file = GetFileTimeStamp(); new_file != old_file){
            log_file_.close();
            log_file_.open(new_file, std::ios::app);
        }
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::string old_file = GetFileTimeStamp();
    std::ofstream log_file_{old_file, std::ios::app};
    std::mutex mutex_;
};
