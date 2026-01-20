#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace web {

using Vars = std::unordered_map<std::string, std::string>;
using Lists = std::unordered_map<std::string, std::vector<Vars>>;

class TemplateEngine {
public:
    std::string render(const std::string& tpl, const Vars& vars, const Lists& lists = {}) const;
};

}
