#pragma once
#include <filesystem>
#include <string_view>
namespace pf {
int run(bool smoke, const std::filesystem::path &visual_output = {},
        std::string_view interaction_test = {},
        const std::filesystem::path &interaction_output = {});
}
