#include "server.hpp"
#include "router.hpp"
#include "template.hpp"
#include "ccss.hpp"
#include "logger.hpp"
#include "module.hpp"
#include "modules/portfolio.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    web::Logger::instance().enable_console(true);
    web::Logger::instance().set_level(web::LogLevel::Info);
    web::Router router;
    router.set_static_dir(STATIC_DIR);
    router.set_template_dir(TEMPLATE_DIR);

    router.add("GET", "/", [&router](const web::Request& req) {
        web::Vars vars{{"title", "Home"}, {"message", "Welcome"}};
        web::Lists lists{
            {"items", {
                web::Vars{{"name", "Alpha"}},
                web::Vars{{"name", "Beta"}},
                web::Vars{{"name", "Gamma"}},
            }}
        };
        return router.render("index.html", vars, lists);
    });

    router.add("GET", "/hello", [](const web::Request& req) {
        std::string name = "World";
        auto it = req.query.find("name");
        if (it != req.query.end() && !it->second.empty()) name = it->second;
        web::Response resp;
        resp.status = 200;
        resp.body = "Hello, " + name;
        resp.headers["Content-Type"] = "text/plain; charset=utf-8";
        return resp;
    });

    router.add("GET", "/assets/main.css", [](const web::Request& req) {
        auto path = web::join_paths(STYLES_DIR, "main.ccss");
        auto src = web::read_file(path);
        web::Response resp;
        if (!src) {
            resp.status = 404;
            resp.body = "Not Found";
            resp.headers["Content-Type"] = "text/plain; charset=utf-8";
            return resp;
        }
        std::unordered_map<std::string,std::string> overrides;
        for (auto it = req.query.begin(); it != req.query.end(); ++it) {
            const std::string& k = it->first;
            if (k.rfind("v_", 0) == 0) {
                overrides[k.substr(2)] = it->second;
            } else if (k.rfind("var_", 0) == 0) {
                overrides[k.substr(4)] = it->second;
            }
        }
        auto css = web::compile_ccss(*src, overrides.empty() ? nullptr : &overrides, STYLES_DIR);
        web::Logger::instance().log(web::LogLevel::Info, "CCSS compiled main.ccss size " + std::to_string(css.size()));
        resp.status = 200;
        resp.body = css;
        resp.headers["Content-Type"] = "text/css; charset=utf-8";
        return resp;
    });

    web::ModuleManager modules;
    modules.load_from_config(router);

    web::Server server("127.0.0.1", 8080, router);
    server.start();
    std::cout << "Server running on http://127.0.0.1:8080/\n";
    std::cout.flush();
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    return 0;
}
