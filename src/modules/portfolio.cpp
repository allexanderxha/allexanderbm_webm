#include "portfolio.hpp"
#include "logger.hpp"
#include <sstream>

namespace web {

static std::string read_all_or(const std::string& path, const std::string& def) {
    auto c = read_file(path);
    if (c) return *c;
    return def;
}

std::string PortfolioModule::name() const {
    return "portfolio";
}

static std::vector<Vars> parse_projects_json(const std::string& json) {
    std::vector<Vars> out;
    size_t i = 0;
    auto skip_ws = [&](void){ while (i < json.size() && (json[i]==' '||json[i]=='\n'||json[i]=='\r'||json[i]=='\t')) ++i; };
    auto expect = [&](char c){ skip_ws(); if (i<json.size() && json[i]==c) { ++i; return true; } return false; };
    auto parse_string = [&]()->std::string {
        skip_ws();
        if (i>=json.size() || json[i] != '"') return "";
        ++i;
        std::ostringstream ss;
        while (i < json.size() && json[i] != '"') {
            if (json[i] == '\\' && i + 1 < json.size()) {
                char x = json[i+1];
                ss << x;
                i += 2;
            } else {
                ss << json[i++];
            }
        }
        if (i < json.size() && json[i] == '"') ++i;
        return ss.str();
    };
    skip_ws();
    if (!expect('[')) return out;
    while (true) {
        skip_ws();
        if (i < json.size() && json[i] == ']') { ++i; break; }
        if (!expect('{')) break;
        Vars obj;
        while (true) {
            skip_ws();
            if (i < json.size() && json[i] == '}') { ++i; break; }
            std::string key = parse_string();
            skip_ws();
            if (i < json.size() && json[i] == ':') ++i;
            skip_ws();
            if (i < json.size() && json[i] == '"') {
                std::string val = parse_string();
                obj[key] = val;
            } else if (i < json.size() && json[i] == '[') {
                ++i;
                std::ostringstream tags;
                bool first = true;
                while (true) {
                    skip_ws();
                    if (i < json.size() && json[i] == ']') { ++i; break; }
                    std::string v = parse_string();
                    if (!first) tags << ", ";
                    first = false;
                    tags << v;
                    skip_ws();
                    if (i < json.size() && json[i] == ',') ++i;
                }
                obj[key] = tags.str();
            } else {
                std::string val;
                while (i < json.size() && json[i] != ',' && json[i] != '}' && json[i] != ']') {
                    val.push_back(json[i++]);
                }
                obj[key] = val;
            }
            skip_ws();
            if (i < json.size() && json[i] == ',') ++i;
        }
        out.push_back(obj);
        skip_ws();
        if (i < json.size() && json[i] == ',') ++i;
    }
    return out;
}

std::vector<Vars> PortfolioModule::load_projects() {
    auto path = join_paths(DATA_DIR, "portfolio.json");
    auto content = read_file(path);
    if (!content) {
        Logger::instance().log(LogLevel::Warn, "portfolio.json not found");
        return {};
    }
    return parse_projects_json(*content);
}

Response PortfolioModule::render_list(Router& router) {
    auto projects = load_projects();
    Lists lists;
    lists["projects"] = {};
    for (auto& p : projects) lists["projects"].push_back(p);
    Vars vars{{"title","Portfolio"}, {"message","Projects"}};
    return router.render("portfolio.html", vars, lists);
}

Response PortfolioModule::render_item(Router& router, const Request& req) {
    std::string id;
    auto it = req.query.find("id");
    if (it != req.query.end()) id = it->second;
    auto projects = load_projects();
    Vars vars{{"title","Project"}, {"message","Project"}};
    Lists lists;
    for (auto& p : projects) {
        auto pit = p.find("id");
        if (pit != p.end() && pit->second == id) {
            lists["item"] = { p };
            break;
        }
    }
    return router.render("portfolio_item.html", vars, lists);
}

void PortfolioModule::register_routes(Router& router) {
    router.add("GET", "/portfolio", [&router](const Request& req){
        return render_list(router);
    });
    router.add("GET", "/portfolio/view", [&router](const Request& req){
        return render_item(router, req);
    });
}

}

