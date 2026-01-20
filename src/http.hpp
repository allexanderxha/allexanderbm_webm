#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace web {

struct Request {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> query;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string raw_target;
};

struct Response {
    int status = 200;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string reason;
    std::string to_string() const;
};

Request parse_request(const std::string& data);
std::string url_decode(const std::string& s);

} // namespace web

