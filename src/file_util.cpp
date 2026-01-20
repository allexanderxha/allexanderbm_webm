#include "file_util.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <string>
#include <vector>

namespace web {

std::optional<std::string> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool file_exists(const std::string& path) {
#if defined(_WIN32)
    struct _stat s;
    return _stat(path.c_str(), &s) == 0;
#else
    struct stat s;
    return stat(path.c_str(), &s) == 0;
#endif
}

static bool ends_with(const std::string& s, const std::string& suf) {
    if (s.size() < suf.size()) return false;
    return std::equal(s.end() - suf.size(), s.end(), suf.begin(), suf.end());
}

std::string guess_mime(const std::string& path) {
    if (ends_with(path, ".html") || ends_with(path, ".htm")) return "text/html; charset=utf-8";
    if (ends_with(path, ".css")) return "text/css; charset=utf-8";
    if (ends_with(path, ".js")) return "application/javascript";
    if (ends_with(path, ".json")) return "application/json";
    if (ends_with(path, ".png")) return "image/png";
    if (ends_with(path, ".jpg") || ends_with(path, ".jpeg")) return "image/jpeg";
    if (ends_with(path, ".gif")) return "image/gif";
    if (ends_with(path, ".svg")) return "image/svg+xml";
    if (ends_with(path, ".ico")) return "image/x-icon";
    if (ends_with(path, ".txt")) return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

std::string join_paths(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    char sep =
#if defined(_WIN32)
        '\\';
#else
        '/';
#endif
    if (a.back() == sep) return a + b;
    return a + sep + b;
}

std::string normalize_rel_path(const std::string& rel) {
    std::vector<std::string> parts;
    std::string cur;
    for (size_t i = 0; i < rel.size(); ++i) {
        char c = rel[i];
        if (c == '/' || c == '\\') {
            if (!cur.empty()) {
                if (cur == ".") { /* skip */ }
                else if (cur == "..") {
                    if (!parts.empty()) parts.pop_back();
                } else {
                    parts.push_back(cur);
                }
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) {
        if (cur == ".") { /* skip */ }
        else if (cur == "..") {
            if (!parts.empty()) parts.pop_back();
        } else {
            parts.push_back(cur);
        }
    }
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out.push_back(
#if defined(_WIN32)
            '\\'
#else
            '/'
#endif
        );
        out += parts[i];
    }
    return out;
}

bool is_safe_relative(const std::string& rel) {
    if (rel.empty()) return false;
    for (char c : rel) {
        if (c == ':' || c == 0) return false;
    }
    auto n = normalize_rel_path(rel);
    return n.find("..") == std::string::npos;
}

}
