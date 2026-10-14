#pragma once
#include <filesystem>
#include <string>

namespace pf {
std::filesystem::path resolve_asset_path(
    const std::string &relative_reference,
    const std::filesystem::path &document_path,
    const std::filesystem::path &resource_root);

std::string ensure_document_asset(const std::filesystem::path &source,
                                  const std::filesystem::path &document_path,
                                  const std::string &relative_reference);
} // namespace pf
