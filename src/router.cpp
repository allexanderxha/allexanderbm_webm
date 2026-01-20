#include "router.hpp"
#include "logger.hpp"
#include <sstream>

namespace web {

static std::string route_key(const std::string& m, const std::string& p) {
    return m + " " + p;
}

void Router::add(const std::string& method, const std::string& path, Handler h) {
    routes_[route_key(method, path)] = std::move(h);
}

void Router::set_static_dir(const std::string& dir) {
    static_dir_ = dir;
}

void Router::set_template_dir(const std::string& dir) {
    template_dir_ = dir;
}

Response Router::render(const std::string& name, const Vars& vars, const Lists& lists) const {
    auto full = join_paths(template_dir_, name);
    auto content_opt = read_file(full);
    Response resp;
    if (!content_opt) {
        Logger::instance().log(LogLevel::Warn, "Template not found: " + full);
        resp.status = 404;
        resp.body = "Template not found";
        resp.headers["Content-Type"] = "text/plain; charset=utf-8";
        return resp;
    }
    auto body = engine_.render(*content_opt, vars, lists);
    resp.status = 200;
    resp.body = body;
    resp.headers["Content-Type"] = "text/html; charset=utf-8";
    return resp;
}

Response Router::route(const Request& r) const {
    auto it = routes_.find(route_key(r.method, r.path));
    if (it != routes_.end()) {
        return it->second(r);
    }
    if (!static_dir_.empty() && r.method == "GET") {
        std::string rel = r.path;
        if (rel == "/") rel = "/index.html";
        rel = normalize_rel_path(rel.substr(1));
        if (!is_safe_relative(rel)) {
            Response bad;
            bad.status = 400;
            bad.body = "Bad Request";
            bad.headers["Content-Type"] = "text/plain; charset=utf-8";
            Logger::instance().log(LogLevel::Warn, "Unsafe path rejected: " + r.path);
            return bad;
        }
        auto full = join_paths(static_dir_, rel);
        if (file_exists(full)) {
            Logger::instance().log(LogLevel::Debug, "Static file: " + full);
            auto content = read_file(full);
            Response resp;
            if (content) {
                resp.status = 200;
                resp.body = *content;
                resp.headers["Content-Type"] = guess_mime(full);
                return resp;
            }
        }
    }
    Response resp;
    resp.status = 404;
    resp.body = "Not Found";
    resp.headers["Content-Type"] = "text/plain; charset=utf-8";
    Logger::instance().log(LogLevel::Warn, "Route not found: " + r.method + " " + r.path);
    return resp;
}

}

