#include "serialization.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <random>
#include <system_error>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#endif
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_set>
namespace pf {
using nlohmann::json;
namespace {
int integer(const json &value) {
  if (!value.is_number_integer() || value.get<double>() < -2147483648.0 ||
      value.get<double>() > 2147483647.0)
    throw std::invalid_argument("Expected bounded integer");
  return value.get<int>();
}
void keys(const json &value, std::initializer_list<std::string_view> allowed,
          const char *where) {
  if (!value.is_object())
    throw std::invalid_argument(std::string(where) + " must be an object");
  for (auto it = value.begin(); it != value.end(); ++it) {
    bool known = false;
    for (auto name : allowed)
      known = known || it.key() == name;
    if (!known)
      throw std::invalid_argument(std::string("Unsupported field in ") + where +
                                  ": " + it.key());
  }
}
void common_root_keys(const json &j) {
  keys(j, {"version", "genre", "width", "height", "layers"}, "version 1 level");
}
void layer_keys(const json &j, int version) {
  if (version == 1)
    keys(j, {"name", "tiles", "visible", "locked"}, "layer");
  else
    keys(j, {"name", "tiles", "visible", "locked"}, "layer");
}
std::filesystem::path temporary_path(const std::filesystem::path &target) {
  static std::atomic<std::uint64_t> sequence{0};
  std::random_device random;
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto suffix = std::to_string(stamp) + "-" +
                      std::to_string(random()) + "-" +
                      std::to_string(sequence.fetch_add(1));
  auto filename = target.filename();
  filename += ".tmp-";
  filename += suffix;
  return target.parent_path().empty()
             ? std::filesystem::path(filename)
             : target.parent_path() / filename;
}
void replace_file(const std::filesystem::path &source,
                  const std::filesystem::path &target) {
#ifdef _WIN32
  if (!MoveFileExW(source.c_str(), target.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::system_error(static_cast<int>(GetLastError()),
                            std::system_category(), "Cannot replace level file");
#else
  if (std::rename(source.c_str(), target.c_str()) != 0)
    throw std::system_error(errno, std::generic_category(),
                            "Cannot replace level file");
#endif
}
} // namespace
std::string serialize(const Level &l) {
  l.validate();
  json j = {{"version", 2},
            {"genre", genre_name(l.genre)},
            {"width", l.width},
            {"height", l.height},
            {"tile_size", {{"width", l.tile_width}, {"height", l.tile_height}}},
            {"tileset",
             {{"path", l.tileset_path},
              {"tile_width", l.atlas_tile_width},
              {"tile_height", l.atlas_tile_height}}},
            {"layers", json::array()},
            {"collision", json::array()},
            {"entities", json::array()}};
  for (const auto &layer : l.layers)
    j["layers"].push_back({{"name", layer.name},
                           {"tiles", layer.tiles},
                           {"visible", layer.visible},
                           {"locked", layer.locked}});
  for (auto value : l.collision)
    j["collision"].push_back(value != 0);
  for (const auto &entity : l.entities)
    j["entities"].push_back({{"id", entity.id},
                             {"type", entity_type_name(entity.type)},
                             {"x", entity.x},
                             {"y", entity.y},
                             {"properties", entity.properties}});
  const auto result = j.dump(2);
  if (result.size() > 16 * 1024 * 1024)
    throw std::invalid_argument("Serialized level exceeds 16 MiB");
  return result;
}
Level deserialize(const std::string &data) {
  if (data.size() > 16 * 1024 * 1024)
    throw std::invalid_argument("Level exceeds 16 MiB");
  const auto j = json::parse(data);
  keys(j,
       {"version", "genre", "width", "height", "layers", "tile_size", "tileset",
        "collision", "entities"},
       "level");
  const int version = integer(j.at("version"));
  if (version != 1 && version != 2)
    throw std::invalid_argument("Unsupported level version");
  if (version == 1)
    common_root_keys(j);
  else if (!j.contains("tile_size") || !j.contains("tileset") ||
           !j.contains("collision") || !j.contains("entities"))
    throw std::invalid_argument("Version 2 level is missing required data");
  Level level(parse_genre(j.at("genre").get<std::string>()),
              integer(j.at("width")), integer(j.at("height")));
  const auto &layers = j.at("layers");
  if (!layers.is_array() || layers.empty() || layers.size() > 16)
    throw std::invalid_argument("Expected 1..16 layers");
  level.layers.clear();
  for (const auto &source : layers) {
    layer_keys(source, version);
    Layer layer{source.at("name").get<std::string>(),
                {},
                source.value("visible", true),
                source.value("locked", false)};
    const auto &tiles = source.at("tiles");
    if (!tiles.is_array() ||
        tiles.size() != static_cast<std::size_t>(level.width * level.height))
      throw std::invalid_argument("Tile count mismatch");
    for (const auto &tile : tiles)
      layer.tiles.push_back(integer(tile));
    level.layers.push_back(std::move(layer));
  }
  if (version == 2) {
    const auto &size = j.at("tile_size");
    keys(size, {"width", "height"}, "tile size");
    level.tile_width = integer(size.at("width"));
    level.tile_height = integer(size.at("height"));
    const auto &tileset = j.at("tileset");
    keys(tileset, {"path", "tile_width", "tile_height"}, "tileset");
    level.tileset_path = tileset.at("path").get<std::string>();
    level.atlas_tile_width = integer(tileset.at("tile_width"));
    level.atlas_tile_height = integer(tileset.at("tile_height"));
    const auto &collision = j.at("collision");
    if (!collision.is_array() ||
        collision.size() !=
            static_cast<std::size_t>(level.width * level.height))
      throw std::invalid_argument("Collision cell count mismatch");
    level.collision.clear();
    for (const auto &cell : collision) {
      if (!cell.is_boolean())
        throw std::invalid_argument("Collision cells must be booleans");
      level.collision.push_back(cell.get<bool>() ? 1 : 0);
    }
    const auto &entities = j.at("entities");
    if (!entities.is_array() || entities.size() > 4096)
      throw std::invalid_argument(
          "Entity list must contain at most 4096 entries");
    for (const auto &source : entities) {
      keys(source, {"id", "type", "x", "y", "properties"}, "entity");
      const auto &id = source.at("id");
      if ((!id.is_number_integer() && !id.is_number_unsigned()) ||
          id.get<long double>() < 1 ||
          id.get<long double>() > 18446744073709551615.0L)
        throw std::invalid_argument(
            "Entity ID must be a positive 64-bit integer");
      const auto &x = source.at("x");
      const auto &y = source.at("y");
      if (!x.is_number() || !y.is_number())
        throw std::invalid_argument("Entity positions must be numbers");
      const auto &properties = source.at("properties");
      if (!properties.is_object())
        throw std::invalid_argument("Entity properties must be an object");
      Entity entity{id.get<std::uint64_t>(),
                    parse_entity_type(source.at("type").get<std::string>()),
                    x.get<float>(),
                    y.get<float>(),
                    {}};
      for (auto it = properties.begin(); it != properties.end(); ++it)
        entity.properties.emplace(it.key(), it.value());
      level.entities.push_back(std::move(entity));
    }
  }
  level.validate();
  return level;
}
void save(const Level &l, const std::filesystem::path &p) {
  const auto data = serialize(l);
  if (p.empty())
    throw std::invalid_argument("Level path cannot be empty");
  const auto temporary = temporary_path(p);
  try {
    {
      std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
      if (!out)
        throw std::runtime_error("Cannot create temporary level file: " +
                                 temporary.string());
      out.write(data.data(), static_cast<std::streamsize>(data.size()));
      out.flush();
      if (!out)
        throw std::runtime_error("Cannot write temporary level file: " +
                                 temporary.string());
      out.close();
      if (!out)
        throw std::runtime_error("Cannot finish temporary level file: " +
                                 temporary.string());
    }
    replace_file(temporary, p);
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    throw;
  }
}
Level load(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary | std::ios::ate);
  if (!in || in.tellg() < 0 || in.tellg() > 16 * 1024 * 1024)
    throw std::runtime_error("Cannot read level (limit 16 MiB)");
  std::string data(static_cast<std::size_t>(in.tellg()), '\0');
  in.seekg(0);
  if (!in.read(data.data(), static_cast<std::streamsize>(data.size())))
    throw std::runtime_error("Incomplete level read");
  return deserialize(data);
}
} // namespace pf
