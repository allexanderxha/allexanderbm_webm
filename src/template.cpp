#include "template.hpp"
#include <sstream>

namespace web {

static std::string replace_all(std::string s, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

static std::string interp(const std::string& s, const Vars& vars) {
    std::string out = s;
    for (auto& [k, v] : vars) {
        out = replace_all(out, "{{" + k + "}}", v);
    }
    return out;
}

static bool find_block(const std::string& tpl, const std::string& name, size_t& begin, size_t& end) {
    auto start_tag = std::string("{% for ") + name + " %}";
    auto end_tag = std::string("{% endfor %}");
    begin = tpl.find(start_tag);
    if (begin == std::string::npos) return false;
    auto body_start = begin + start_tag.size();
    end = tpl.find(end_tag, body_start);
    if (end == std::string::npos) return false;
    return true;
}

std::string TemplateEngine::render(const std::string& tpl, const Vars& vars, const Lists& lists) const {
    std::string out = interp(tpl, vars);
    for (auto& [list_name, rows] : lists) {
        size_t b{}, e{};
        if (find_block(out, list_name, b, e)) {
            auto start_tag = std::string("{% for ") + list_name + " %}";
            auto body_start = b + start_tag.size();
            auto body = out.substr(body_start, e - body_start);
            std::ostringstream built;
            for (auto& row : rows) {
                built << interp(body, row);
            }
            out = out.substr(0, b) + built.str() + out.substr(e + std::string("{% endfor %}").size());
        }
    }
    return out;
}

}
