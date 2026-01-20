#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <atomic>

namespace web {

enum class LogLevel { Trace, Debug, Info, Warn, Error };

class Logger {
public:
    static Logger& instance();
    void set_level(LogLevel lvl);
    LogLevel get_level() const;
    const char* level_name(LogLevel lvl);
    void enable_console(bool enabled);
    void set_file(const std::string& path, std::size_t max_bytes);
    void log(LogLevel lvl, const std::string& msg);
private:
    Logger() = default;
    std::mutex mtx_;
    std::ofstream file_;
    std::string file_path_;
    std::size_t max_bytes_{0};
    std::atomic<LogLevel> level_{LogLevel::Info};
    std::atomic<bool> console_{true};
    void write_line(const std::string& line, LogLevel lvl);
    std::string timestamp();
    void rotate_if_needed(std::size_t append_len);
};

}

