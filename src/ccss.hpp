#pragma once
#include <string>
#include <unordered_map>

namespace web {

std::string compile_ccss(const std::string& source, const std::unordered_map<std::string,std::string>* overrides = nullptr);
std::string compile_ccss(const std::string& source, const std::unordered_map<std::string,std::string>* overrides, const std::string& base_dir);

}
