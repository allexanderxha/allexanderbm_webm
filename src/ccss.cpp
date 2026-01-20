#include "ccss.hpp"
#include "file_util.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <set>

namespace web {

static std::string trim(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e-1]))) --e;
    return s.substr(b, e - b);
}

static std::string replace_vars(std::string s, const std::unordered_map<std::string,std::string>& vars) {
    for (auto it = vars.begin(); it != vars.end(); ++it) {
        const std::string k = "$" + it->first;
        size_t pos = 0;
        while ((pos = s.find(k, pos)) != std::string::npos) {
            s.replace(pos, k.size(), it->second);
            pos += it->second.size();
        }
    }
    return s;
}

static bool parse_hex_color(const std::string& hex, int& r, int& g, int& b) {
    if (hex.size() == 7 && hex[0] == '#') {
        r = std::stoi(hex.substr(1,2), nullptr, 16);
        g = std::stoi(hex.substr(3,2), nullptr, 16);
        b = std::stoi(hex.substr(5,2), nullptr, 16);
        return true;
    }
    if (hex.size() == 4 && hex[0] == '#') {
        int rr = std::stoi(std::string(2, hex[1]), nullptr, 16);
        int gg = std::stoi(std::string(2, hex[2]), nullptr, 16);
        int bb = std::stoi(std::string(2, hex[3]), nullptr, 16);
        r = rr; g = gg; b = bb;
        return true;
    }
    return false;
}

static std::string to_hex(int r, int g, int b) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    return std::string(buf);
}

static std::string eval_functions(const std::string& s) {
    std::string out = s;
    size_t pos = 0;
    while ((pos = out.find("lighten(", pos)) != std::string::npos) {
        size_t start = pos + 8;
        size_t end = out.find(')', start);
        if (end == std::string::npos) break;
        std::string args = trim(out.substr(start, end - start));
        size_t comma = args.find(',');
        if (comma == std::string::npos) { pos = end + 1; continue; }
        std::string c = trim(args.substr(0, comma));
        std::string p = trim(args.substr(comma + 1));
        int r,g,b;
        if (!parse_hex_color(c, r, g, b)) { pos = end + 1; continue; }
        int percent = 0;
        if (!p.empty() && p.back() == '%') p.pop_back();
        try { percent = std::stoi(p); } catch (...) { percent = 0; }
        auto inc = [&](int x){ return x + (255 - x) * percent / 100; };
        r = std::min(255, std::max(0, inc(r)));
        g = std::min(255, std::max(0, inc(g)));
        b = std::min(255, std::max(0, inc(b)));
        std::string rep = to_hex(r,g,b);
        out.replace(pos, (end - pos) + 1, rep);
    }
    pos = 0;
    while ((pos = out.find("darken(", pos)) != std::string::npos) {
        size_t start = pos + 7;
        size_t end = out.find(')', start);
        if (end == std::string::npos) break;
        std::string args = trim(out.substr(start, end - start));
        size_t comma = args.find(',');
        if (comma == std::string::npos) { pos = end + 1; continue; }
        std::string c = trim(args.substr(0, comma));
        std::string p = trim(args.substr(comma + 1));
        int r,g,b;
        if (!parse_hex_color(c, r, g, b)) { pos = end + 1; continue; }
        int percent = 0;
        if (!p.empty() && p.back() == '%') p.pop_back();
        try { percent = std::stoi(p); } catch (...) { percent = 0; }
        auto dec = [&](int x){ return x - x * percent / 100; };
        r = std::min(255, std::max(0, dec(r)));
        g = std::min(255, std::max(0, dec(g)));
        b = std::min(255, std::max(0, dec(b)));
        std::string rep = to_hex(r,g,b);
        out.replace(pos, (end - pos) + 1, rep);
    }
    return out;
}

static bool safe_path(const std::string& p) {
    if (p.find("..") != std::string::npos) return false;
    if (!p.empty() && (p[0] == '/' || p[0] == '\\')) return false;
    return true;
}

static std::string expand_imports(const std::string& src, const std::string& base_dir, std::set<std::string>& visited) {
    std::string s = src;
    size_t pos = 0;
    while ((pos = s.find("@import", pos)) != std::string::npos) {
        size_t p = pos + 7;
        while (p < s.size() && std::isspace(static_cast<unsigned char>(s[p]))) ++p;
        if (p >= s.size()) break;
        char quote = 0;
        if (s[p] == '\'' || s[p] == '"') {
            quote = s[p++];
        } else {
            break;
        }
        size_t start = p;
        while (p < s.size() && s[p] != quote) ++p;
        if (p >= s.size()) break;
        std::string rel = s.substr(start, p - start);
        size_t endq = p + 1;
        while (endq < s.size() && s[endq] != ';') ++endq;
        if (!safe_path(rel)) {
            s.erase(pos, (endq > pos ? endq - pos + 1 : 7));
            continue;
        }
        std::string filename = rel;
        if (filename.find('.') == std::string::npos) filename += ".ccss";
        std::string full = join_paths(base_dir, filename);
        if (visited.count(full)) {
            s.erase(pos, (endq > pos ? endq - pos + 1 : 7));
            continue;
        }
        visited.insert(full);
        auto content = read_file(full);
        std::string replacement;
        if (content) {
            replacement = expand_imports(*content, base_dir, visited);
        }
        s.replace(pos, (endq > pos ? endq - pos + 1 : 7), replacement);
    }
    return s;
}

struct Mixin {
    std::vector<std::string> params;
    std::string body;
};

static std::string apply_mixin(const Mixin& m, const std::vector<std::string>& args, const std::unordered_map<std::string,std::string>& vars) {
    std::unordered_map<std::string,std::string> local = vars;
    for (size_t i = 0; i < m.params.size() && i < args.size(); ++i) {
        local[m.params[i]] = args[i];
    }
    std::string v = replace_vars(m.body, local);
    v = eval_functions(v);
    return v;
}

std::string compile_ccss(const std::string& source, const std::unordered_map<std::string,std::string>* overrides, const std::string& base_dir) {
    std::set<std::string> visited;
    std::string src = expand_imports(source, base_dir, visited);
    std::vector<std::unordered_map<std::string,std::string>> var_stack;
    var_stack.emplace_back();
    if (overrides) {
        for (auto it = overrides->begin(); it != overrides->end(); ++it) {
            var_stack.back()[it->first] = it->second;
        }
    }
    std::unordered_map<std::string,Mixin> mixins;
    std::vector<std::string> sel_stack;
    std::unordered_map<std::string, std::vector<std::pair<std::string,std::string>>> rules;
    std::string token;
    std::string cur_selector;
    std::string s = src;
    size_t i = 0;
    while (i < s.size()) {
        if (std::isspace(static_cast<unsigned char>(s[i]))) { ++i; continue; }
        if (s[i] == '$') {
            size_t j = i + 1;
            while (j < s.size() && (std::isalnum(static_cast<unsigned char>(s[j])) || s[j] == '_' || s[j] == '-')) ++j;
            std::string name = s.substr(i+1, j-(i+1));
            while (j < s.size() && std::isspace(static_cast<unsigned char>(s[j]))) ++j;
            if (j < s.size() && s[j] == ':') ++j;
            while (j < s.size() && std::isspace(static_cast<unsigned char>(s[j]))) ++j;
            size_t k = j;
            while (k < s.size() && s[k] != ';') ++k;
            std::string value = trim(s.substr(j, k-j));
            if (var_stack.empty()) var_stack.emplace_back();
            var_stack.back()[name] = value;
            i = k + 1;
            continue;
        }
        if (s[i] == '@') {
            size_t j = i + 1;
            while (j < s.size() && std::isalpha(static_cast<unsigned char>(s[j]))) ++j;
            std::string at = s.substr(i+1, j-(i+1));
            if (at == "mixin") {
                while (j < s.size() && std::isspace(static_cast<unsigned char>(s[j]))) ++j;
                size_t k = j;
                while (k < s.size() && (std::isalnum(static_cast<unsigned char>(s[k])) || s[k]=='_' || s[k]=='-')) ++k;
                std::string name = s.substr(j, k-j);
                while (k < s.size() && std::isspace(static_cast<unsigned char>(s[k]))) ++k;
                std::vector<std::string> params;
                if (k < s.size() && s[k] == '(') {
                    ++k;
                    size_t pstart = k;
                    int depth = 1;
                    while (k < s.size() && depth > 0) {
                        if (s[k] == '(') ++depth;
                        else if (s[k] == ')') --depth;
                        ++k;
                    }
                    std::string plist = trim(s.substr(pstart, (k-1) - pstart));
                    std::stringstream pss(plist);
                    std::string pv;
                    while (std::getline(pss, pv, ',')) {
                        pv = trim(pv);
                        if (!pv.empty() && pv[0] == '$') pv = pv.substr(1);
                        if (!pv.empty()) params.push_back(pv);
                    }
                }
                while (k < s.size() && std::isspace(static_cast<unsigned char>(s[k]))) ++k;
                if (k < s.size() && s[k] == '{') {
                    ++k;
                    size_t bstart = k;
                    int depth = 1;
                    while (k < s.size() && depth > 0) {
                        if (s[k] == '{') ++depth;
                        else if (s[k] == '}') --depth;
                        ++k;
                    }
                    std::string body = trim(s.substr(bstart, (k-1) - bstart));
                    mixins[name] = Mixin{params, body};
                    i = k;
                    continue;
                }
            }
            if (at == "include") {
                while (j < s.size() && std::isspace(static_cast<unsigned char>(s[j]))) ++j;
                size_t k = j;
                while (k < s.size() && (std::isalnum(static_cast<unsigned char>(s[k])) || s[k]=='_' || s[k]=='-')) ++k;
                std::string name = s.substr(j, k-j);
                while (k < s.size() && std::isspace(static_cast<unsigned char>(s[k]))) ++k;
                std::vector<std::string> args;
                if (k < s.size() && s[k] == '(') {
                    ++k;
                    size_t astart = k;
                    int depth = 1;
                    while (k < s.size() && depth > 0) {
                        if (s[k] == '(') ++depth;
                        else if (s[k] == ')') --depth;
                        ++k;
                    }
                    std::string alist = trim(s.substr(astart, (k-1) - astart));
                    std::stringstream ass(alist);
                    std::string av;
                    while (std::getline(ass, av, ',')) {
                        av = trim(av);
                        args.push_back(av);
                    }
                }
                while (k < s.size() && s[k] != ';' && s[k] != '\n' && s[k] != '\r') ++k;
                auto mit = mixins.find(name);
                if (mit != mixins.end()) {
                    std::unordered_map<std::string,std::string> cur = var_stack.empty() ? std::unordered_map<std::string,std::string>{} : var_stack.back();
                    std::string exp = apply_mixin(mit->second, args, cur);
                    std::stringstream es(exp);
                    std::string line;
                    while (std::getline(es, line, ';')) {
                        line = trim(line);
                        if (line.empty()) continue;
                        auto pos = line.find(':');
                        if (pos == std::string::npos) continue;
                        std::string prop = trim(line.substr(0, pos));
                        std::string val = trim(line.substr(pos + 1));
                        val = replace_vars(val, cur);
                        val = eval_functions(val);
                        std::string sel = sel_stack.empty() ? "" : sel_stack.back();
                        rules[sel].push_back({prop, val});
                    }
                }
                i = (k < s.size() && s[k] == ';') ? k+1 : k;
                continue;
            }
            ++i;
            continue;
        }
        size_t j = i;
        while (j < s.size() && s[j] != '{' && s[j] != ';' && s[j] != '}') ++j;
        if (j < s.size() && s[j] == '{') {
            std::string selector = trim(s.substr(i, j-i));
            std::string parent = sel_stack.empty() ? "" : sel_stack.back();
            if (!parent.empty()) {
                if (!selector.empty() && selector[0] == '&') {
                    selector = parent + selector.substr(1);
                } else {
                    selector = parent + " " + selector;
                }
            }
            sel_stack.push_back(selector);
            var_stack.push_back(var_stack.empty() ? std::unordered_map<std::string,std::string>{} : var_stack.back());
            i = j + 1;
            continue;
        }
        if (j < s.size() && s[j] == '}') {
            if (!sel_stack.empty()) sel_stack.pop_back();
            if (!var_stack.empty()) var_stack.pop_back();
            i = j + 1;
            continue;
        }
        if (j < s.size() && s[j] == ';') {
            std::string decl = trim(s.substr(i, j-i));
            auto pos = decl.find(':');
            if (pos != std::string::npos) {
                std::string prop = trim(decl.substr(0, pos));
                std::string val = trim(decl.substr(pos + 1));
                std::unordered_map<std::string,std::string> cur = var_stack.empty() ? std::unordered_map<std::string,std::string>{} : var_stack.back();
                val = replace_vars(val, cur);
                val = eval_functions(val);
                std::string sel = sel_stack.empty() ? "" : sel_stack.back();
                rules[sel].push_back({prop, val});
            }
            i = j + 1;
            continue;
        }
        ++i;
    }
    std::ostringstream out;
    for (auto it = rules.begin(); it != rules.end(); ++it) {
        if (it->first.empty()) continue;
        out << it->first << " {\n";
        for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
            out << "  " << jt->first << ": " << jt->second << ";\n";
        }
        out << "}\n";
    }
    return out.str();
}

std::string compile_ccss(const std::string& source, const std::unordered_map<std::string,std::string>* overrides) {
#ifdef STYLES_DIR
    return compile_ccss(source, overrides, STYLES_DIR);
#else
    return compile_ccss(source, overrides, ".");
#endif
}

}
