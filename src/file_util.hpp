#pragma once
#include <string>
#include <optional>

namespace web {
std::optional<std::string> read_file(const std::string& path);
std::string guess_mime(const std::string& path);
std::string join_paths(const std::string& a, const std::string& b);
bool file_exists(const std::string& path);
std::string normalize_rel_path(const std::string& rel);
bool is_safe_relative(const std::string& rel);
}

