#include "asset_paths.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <stdexcept>

namespace pf {
namespace {
std::filesystem::path checked_relative(const std::string &reference) {
  if (reference.empty() || reference.find('\0') != std::string::npos ||
      reference.find('\\') != std::string::npos ||
      reference.find(':') != std::string::npos)
    throw std::invalid_argument("Asset reference must be a safe relative path");
  const std::filesystem::path relative(reference);
  if (relative.has_root_path())
    throw std::invalid_argument("Asset reference must be relative");
  for (const auto &part : relative)
    if (part == ".." || part == ".")
      throw std::invalid_argument("Asset reference cannot traverse directories");
  return relative;
}
bool same_contents(const std::filesystem::path &a,
                   const std::filesystem::path &b) {
  std::error_code error;
  if (std::filesystem::equivalent(a, b, error) && !error)
    return true;
  const auto first_size = std::filesystem::file_size(a, error);
  if (error)
    return false;
  const auto second_size = std::filesystem::file_size(b, error);
  if (error || first_size != second_size)
    return false;
  std::ifstream first(a, std::ios::binary), second(b, std::ios::binary);
  if (!first || !second)
    return false;
  char first_buffer[8192], second_buffer[8192];
  while (first && second) {
    first.read(first_buffer, sizeof(first_buffer));
    second.read(second_buffer, sizeof(second_buffer));
    const auto first_count = first.gcount();
    const auto second_count = second.gcount();
    if (first_count != second_count ||
        !std::equal(first_buffer, first_buffer + first_count, second_buffer))
      return false;
  }
  return first.eof() && second.eof();
}
bool contained_path(const std::filesystem::path &root,
                    const std::filesystem::path &candidate) {
  const auto relative = candidate.lexically_relative(root);
  if (relative.empty() && candidate != root)
    return false;
  if (relative.is_absolute())
    return false;
  for (const auto &part : relative)
    if (part == "..")
      return false;
  return true;
}
std::filesystem::path choose_destination(
    const std::filesystem::path &parent,
    const std::filesystem::path &relative, const std::filesystem::path &source) {
  auto destination = parent / relative;
  if (!std::filesystem::exists(destination) ||
      same_contents(source, destination))
    return destination;

  const auto original_name = relative.filename();
  const auto stem = original_name.stem().string();
  const auto extension = original_name.extension().string();
  for (unsigned index = 1; index < 10000; ++index) {
    auto filename = std::filesystem::path(stem + "-" +
                                          std::to_string(index) +
                                          extension);
    auto candidate = destination.parent_path() / filename;
    if (!std::filesystem::exists(candidate) ||
        same_contents(source, candidate))
      return candidate;
  }
  throw std::runtime_error("Cannot choose a unique document asset filename");
}
} // namespace

std::filesystem::path resolve_asset_path(
    const std::string &relative_reference,
    const std::filesystem::path &document_path,
    const std::filesystem::path &resource_root) {
  const auto relative = checked_relative(relative_reference);
  if (!document_path.empty()) {
    const auto root = std::filesystem::weakly_canonical(
        std::filesystem::absolute(document_path.parent_path()));
    const auto local = root / relative;
    if (std::filesystem::is_regular_file(local)) {
      const auto canonical_local = std::filesystem::weakly_canonical(local);
      if (!contained_path(root, canonical_local))
        throw std::runtime_error("Tileset asset escapes the document folder");
      return canonical_local;
    }
  }
  if (!resource_root.empty()) {
    const auto root = std::filesystem::weakly_canonical(
        std::filesystem::absolute(resource_root));
    const auto bundled = root / relative;
    if (std::filesystem::is_regular_file(bundled)) {
      const auto canonical_bundled = std::filesystem::weakly_canonical(bundled);
      if (!contained_path(root, canonical_bundled))
        throw std::runtime_error("Bundled tileset escapes the resource folder");
      return canonical_bundled;
    }
  }
  throw std::runtime_error("Tileset asset was not found: " + relative_reference);
}

std::string ensure_document_asset(const std::filesystem::path &source,
                                  const std::filesystem::path &document_path,
                                  const std::string &relative_reference) {
  const auto relative = checked_relative(relative_reference);
  if (document_path.empty() || source.empty() ||
      !std::filesystem::is_regular_file(source))
    throw std::runtime_error("Cannot package the selected tileset asset");
  const auto document_parent =
      std::filesystem::absolute(document_path.parent_path().empty()
                                    ? std::filesystem::current_path()
                                    : document_path.parent_path());
  const auto target_parent = document_parent / relative.parent_path();
  std::filesystem::create_directories(target_parent);
  const auto canonical_root = std::filesystem::weakly_canonical(document_parent);
  const auto canonical_target_parent =
      std::filesystem::weakly_canonical(target_parent);
  if (!contained_path(canonical_root, canonical_target_parent))
    throw std::runtime_error("Tileset target escapes the document directory");

  const auto destination =
      choose_destination(document_parent, relative, source);
  const auto canonical_destination =
      std::filesystem::weakly_canonical(destination);
  if (!contained_path(canonical_root, canonical_destination))
    throw std::runtime_error("Tileset file escapes the document directory");
  if (same_contents(source, destination))
    return destination.lexically_relative(document_parent).generic_string();

  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto temporary = destination;
  temporary += ".tmp-" + std::to_string(stamp);
  try {
    std::filesystem::copy_file(source, temporary,
                               std::filesystem::copy_options::none);
    std::filesystem::rename(temporary, destination);
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    throw;
  }
  return destination.lexically_relative(document_parent).generic_string();
}
} // namespace pf
