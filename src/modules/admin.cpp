#include "admin.hpp"
#include "http.hpp"

namespace web {

std::string AdminModule::name() const {
    return "admin";
}

LogLevel AdminModule::parse_level(const std::string& s) {
    if (s == "TRACE" || s == "Trace" || s == "trace") return LogLevel::Trace;
    if (s == "DEBUG" || s == "Debug" || s == "debug") return LogLevel::Debug;
    if (s == "INFO"  || s == "Info"  || s == "info")  return LogLevel::Info;
    if (s == "WARN"  || s == "Warn"  || s == "warn")  return LogLevel::Warn;
    if (s == "ERROR" || s == "Error" || s == "error") return LogLevel::Error;
    return LogLevel::Info;
}

void AdminModule::register_routes(Router& router) {
    router.add("GET", "/admin/log/level", [this](const Request& req){
        Response r;
        r.headers["Content-Type"] = "text/plain; charset=utf-8";
        auto it = req.query.find("get");
        if (it != req.query.end()) {
            r.status = 200;
            r.body = Logger::instance().level_name(Logger::instance().get_level());
            return r;
        }
        auto sit = req.query.find("set");
        if (sit != req.query.end()) {
            Logger::instance().set_level(parse_level(sit->second));
            r.status = 200;
            r.body = "OK";
            return r;
        }
        r.status = 400;
        r.body = "Bad Request";
        return r;
    });
    router.add("GET", "/admin/info", [this](const Request& req){
        Response r;
        r.status = 200;
        r.headers["Content-Type"] = "text/plain; charset=utf-8";
        std::string body = "modules:";
        auto names = mgr_.list_names();
        for (auto& n : names) {
            body += " " + n;
        }
        r.body = body;
        return r;
    });
}

}

