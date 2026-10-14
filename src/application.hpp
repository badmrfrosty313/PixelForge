#pragma once
#include <filesystem>
namespace pf {
int run(bool smoke, const std::filesystem::path &visual_output = {});
}
