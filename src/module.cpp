#include "module.hpp"
#include "logger.hpp"
#include "file_util.hpp"
#include "modules/portfolio.hpp"
#include "modules/health.hpp"
#include "modules/admin.hpp"
#include <sstream>

namespace web {

void ModuleManager::add(std::unique_ptr<Module> m) {
    mods_.push_back(std::move(m));
}

void ModuleManager::init(Router& router) {
    for (auto& m : mods_) {
        m->register_routes(router);
        Logger::instance().log(LogLevel::Info, "Module loaded: " + m->name());
    }
}

void ModuleManager::load_from_config(Router& router) {
    auto path = join_paths(DATA_DIR, "modules.json");
    auto content = read_file(path);
    std::vector<std::string> names;
    if (content) {
        std::string s = *content;
        size_t i = 0;
        auto skip = [&]{ while (i < s.size() && (s[i]==' '||s[i]=='\n'||s[i]=='\r'||s[i]=='\t')) ++i; };
        auto expect = [&](char c){ skip(); if (i<s.size() && s[i]==c){ ++i; return true;} return false; };
        auto parse_string = [&]()->std::string {
            skip();
            if (i>=s.size() || s[i] != '"') return "";
            ++i;
            std::ostringstream ss;
            while (i < s.size() && s[i] != '"') {
                if (s[i] == '\\' && i + 1 < s.size()) {
                    ss << s[i+1];
                    i += 2;
                } else {
                    ss << s[i++];
                }
            }
            if (i < s.size() && s[i] == '"') ++i;
            return ss.str();
        };
        skip();
        if (expect('{')) {
            while (true) {
                skip();
                if (i < s.size() && s[i] == '}') { ++i; break; }
                std::string key = parse_string();
                skip();
                if (i < s.size() && s[i] == ':') ++i;
                skip();
                if (key == "modules" && i < s.size() && s[i] == '[') {
                    ++i;
                    while (true) {
                        skip();
                        if (i < s.size() && s[i] == ']') { ++i; break; }
                        std::string v = parse_string();
                        if (!v.empty()) names.push_back(v);
                        skip();
                        if (i < s.size() && s[i] == ',') ++i;
                    }
                } else {
                    while (i < s.size() && s[i] != ',' && s[i] != '}') ++i;
                }
                skip();
                if (i < s.size() && s[i] == ',') ++i;
            }
        }
    }
    if (names.empty()) {
        add(std::make_unique<PortfolioModule>());
        add(std::make_unique<HealthModule>());
        add(std::make_unique<AdminModule>(*this));
    } else {
        for (auto& n : names) {
            if (n == "portfolio") add(std::make_unique<PortfolioModule>());
            else if (n == "health") add(std::make_unique<HealthModule>());
            else if (n == "admin") add(std::make_unique<AdminModule>(*this));
        }
    }
    init(router);
}

std::vector<std::string> ModuleManager::list_names() const {
    std::vector<std::string> out;
    for (auto& m : mods_) out.push_back(m->name());
    return out;
}

}
