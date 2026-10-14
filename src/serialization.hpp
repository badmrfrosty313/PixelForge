#pragma once
#include "level.hpp"
#include <filesystem>
namespace pf {
std::string serialize(const Level &level);
Level deserialize(const std::string &data);
void save(const Level &level, const std::filesystem::path &path);
Level load(const std::filesystem::path &path);
} // namespace pf
