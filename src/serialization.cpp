#include "serialization.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace pf {
using nlohmann::json;
std::string serialize(const Level &l) {
  l.validate();
  json j = {{"version", 1},
            {"genre", genre_name(l.genre)},
            {"width", l.width},
            {"height", l.height},
            {"layers", json::array()}};
  for (const auto &layer : l.layers)
    j["layers"].push_back({{"name", layer.name},
                           {"tiles", layer.tiles},
                           {"visible", layer.visible},
                           {"locked", layer.locked}});
  return j.dump(2);
}
int integer(const json &v) {
  if (!v.is_number_integer() || v.get<double>() < -2147483648.0 ||
      v.get<double>() > 2147483647.0)
    throw std::invalid_argument("Expected bounded integer");
  return v.get<int>();
}
Level deserialize(const std::string &data) {
  if (data.size() > 16 * 1024 * 1024)
    throw std::invalid_argument("Level exceeds 16 MiB");
  auto j = json::parse(data);
  if (integer(j.at("version")) != 1)
    throw std::invalid_argument("Unsupported level version");
  Level l(parse_genre(j.at("genre").get<std::string>()), integer(j.at("width")),
          integer(j.at("height")));
  const auto &a = j.at("layers");
  if (!a.is_array() || a.empty() || a.size() > 16)
    throw std::invalid_argument("Expected 1..16 layers");
  l.layers.clear();
  for (const auto &v : a) {
    Layer layer{v.at("name").get<std::string>(),
                {},
                v.value("visible", true),
                v.value("locked", false)};
    const auto &tiles = v.at("tiles");
    if (!tiles.is_array() ||
        tiles.size() != static_cast<std::size_t>(l.width * l.height))
      throw std::invalid_argument("Tile count mismatch");
    for (const auto &t : tiles)
      layer.tiles.push_back(integer(t));
    l.layers.push_back(std::move(layer));
  }
  l.validate();
  return l;
}
void save(const Level &l, const std::filesystem::path &p) {
  const auto data = serialize(l);
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  if (!out || !(out << data) || !out.flush())
    throw std::runtime_error("Cannot write level: " + p.string());
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
