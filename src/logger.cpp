#include "logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace web {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::set_level(LogLevel lvl) {
    level_ = lvl;
}

LogLevel Logger::get_level() const {
    return level_.load();
}

void Logger::enable_console(bool enabled) {
    console_ = enabled;
}

void Logger::set_file(const std::string& path, std::size_t max_bytes) {
    std::lock_guard<std::mutex> lk(mtx_);
    file_path_ = path;
    max_bytes_ = max_bytes;
    if (file_.is_open()) file_.close();
    file_.open(file_path_, std::ios::app | std::ios::out);
}

const char* Logger::level_name(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "INFO";
    }
}

std::string Logger::timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms.count();
    return ss.str();
}

void Logger::rotate_if_needed(std::size_t append_len) {
    if (!file_.is_open() || max_bytes_ == 0) return;
    file_.seekp(0, std::ios::end);
    auto size = static_cast<std::size_t>(file_.tellp());
    if (size + append_len < max_bytes_) return;
    file_.close();
    std::string rotated = file_path_ + ".1";
#if defined(_WIN32)
    DeleteFileA(rotated.c_str());
    MoveFileA(file_path_.c_str(), rotated.c_str());
#else
    std::remove(rotated.c_str());
    std::rename(file_path_.c_str(), rotated.c_str());
#endif
    file_.open(file_path_, std::ios::out | std::ios::trunc);
}

void Logger::write_line(const std::string& line, LogLevel lvl) {
    std::lock_guard<std::mutex> lk(mtx_);
    rotate_if_needed(line.size() + 1);
    if (file_.is_open()) {
        file_ << line << "\n";
        file_.flush();
    }
    if (console_) {
#if defined(_WIN32)
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color = 7;
        switch (lvl) {
            case LogLevel::Trace: color = 8; break;
            case LogLevel::Debug: color = 7; break;
            case LogLevel::Info:  color = 7; break;
            case LogLevel::Warn:  color = 6; break;
            case LogLevel::Error: color = 12; break;
        }
        CONSOLE_SCREEN_BUFFER_INFO info{};
        GetConsoleScreenBufferInfo(h, &info);
        SetConsoleTextAttribute(h, color);
        printf("%s\n", line.c_str());
        SetConsoleTextAttribute(h, info.wAttributes);
#else
        const char* pre = "";
        const char* post = "\033[0m";
        switch (lvl) {
            case LogLevel::Trace: pre = "\033[90m"; break;
            case LogLevel::Debug: pre = "\033[37m"; break;
            case LogLevel::Info:  pre = "\033[0m"; break;
            case LogLevel::Warn:  pre = "\033[33m"; break;
            case LogLevel::Error: pre = "\033[31m"; break;
        }
        fprintf(stdout, "%s%s%s\n", pre, line.c_str(), post);
#endif
    }
}

void Logger::log(LogLevel lvl, const std::string& msg) {
    if (static_cast<int>(lvl) < static_cast<int>(level_.load())) return;
    std::ostringstream ss;
    ss << timestamp() << " [" << level_name(lvl) << "] " << msg;
    write_line(ss.str(), lvl);
}

}
