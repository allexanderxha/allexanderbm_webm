#include "http.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace web {

static std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::unordered_map<std::string, std::string> parse_query(const std::string& q) {
    std::unordered_map<std::string, std::string> m;
    std::stringstream ss(q);
    std::string kv;
    while (std::getline(ss, kv, '&')) {
        auto pos = kv.find('=');
        if (pos == std::string::npos) {
            m[url_decode(kv)] = "";
        } else {
            m[url_decode(kv.substr(0, pos))] = url_decode(kv.substr(pos + 1));
        }
    }
    return m;
}

Request parse_request(const std::string& data) {
    Request r;
    std::istringstream stream(data);
    std::string line;
    if (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream l(line);
        std::string target;
        l >> r.method >> target;
        r.raw_target = target;
        auto qpos = target.find('?');
        if (qpos == std::string::npos) {
            r.path = target;
        } else {
            r.path = target.substr(0, qpos);
            r.query = parse_query(target.substr(qpos + 1));
        }
    }
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        auto pos = line.find(':');
        if (pos != std::string::npos) {
            auto k = trim(line.substr(0, pos));
            auto v = trim(line.substr(pos + 1));
            r.headers[k] = v;
        }
    }
    std::string body;
    std::getline(stream, body, '\0');
    r.body = body;
    return r;
}

static std::string reason_phrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default: return "OK";
    }
}

static std::string http_date() {
    std::time_t t = std::time(nullptr);
    std::tm gm{};
#if defined(_WIN32)
    gmtime_s(&gm, &t);
#else
    gmtime_r(&t, &gm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gm);
    return std::string(buf);
}

std::string Response::to_string() const {
    std::ostringstream out;
    std::string r = reason.empty() ? reason_phrase(status) : reason;
    out << "HTTP/1.1 " << status << " " << r << "\r\n";
    auto it = headers.find("Content-Length");
    if (it == headers.end()) {
        out << "Content-Length: " << body.size() << "\r\n";
    }
    if (headers.find("Date") == headers.end()) {
        out << "Date: " << http_date() << "\r\n";
    }
    if (headers.find("Server") == headers.end()) {
        out << "Server: WebServerEngine/1.0\r\n";
    }
    if (headers.find("X-Content-Type-Options") == headers.end()) {
        out << "X-Content-Type-Options: nosniff\r\n";
    }
    for (auto it2 = headers.begin(); it2 != headers.end(); ++it2) {
        out << it2->first << ": " << it2->second << "\r\n";
    }
    out << "\r\n";
    out << body;
    return out.str();
}

std::string url_decode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            auto hex = s.substr(i + 1, 2);
            int val = std::stoi(hex, nullptr, 16);
            out.push_back(static_cast<char>(val));
            i += 2;
        } else if (s[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

} // namespace web
