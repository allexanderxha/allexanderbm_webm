#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace web {

using Vars = std::unordered_map<std::string, std::string>;
using Lists = std::unordered_map<std::string, std::vector<Vars>>;

// Support for nested lists in template variables
struct NestedVar {
    std::string str_val;
    std::vector<Vars> list_val;
    bool is_list = false;
};

using VarsWithNested = std::unordered_map<std::string, NestedVar>;

class TemplateEngine {
public:
    std::string render(const std::string& tpl, const Vars& vars, const Lists& lists = {}) const;
    std::string render_nested(const std::string& tpl, const VarsWithNested& vars, const Lists& lists = {}) const;
};

}
