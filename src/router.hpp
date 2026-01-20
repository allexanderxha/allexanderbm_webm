#pragma once
#include "http.hpp"
#include "file_util.hpp"
#include "template.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace web {

using Handler = std::function<Response(const Request&)>;

class Router {
public:
    void add(const std::string& method, const std::string& path, Handler h);
    void set_static_dir(const std::string& dir);
    void set_template_dir(const std::string& dir);
    Response route(const Request& r) const;
    Response render(const std::string& name, const Vars& vars, const Lists& lists = {}) const;
private:
    std::unordered_map<std::string, Handler> routes_;
    std::string static_dir_;
    std::string template_dir_;
    TemplateEngine engine_;
};

}

