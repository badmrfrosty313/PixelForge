#include "serialization.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
int checks = 0;
void check(bool ok) {
  ++checks;
  if (!ok)
    throw std::runtime_error("Check failed: " + std::to_string(checks));
}
template <class F> void rejects(F f) {
  bool caught = false;
  try {
    f();
  } catch (const std::exception &) {
    caught = true;
  }
  check(caught);
}
} // namespace
int main(int argc, char **argv) {
  try {
    using namespace pf;
    for (auto genre : {Genre::top_down_rpg, Genre::platformer}) {
      Level l(genre, 4, 3);
      check(l.tile(0, 0, 0) == 0);
      l.paint(0, 3, 2, 255);
      check(l.tile(0, 3, 2) == 255);
      l.paint(0, 3, 2, 0);
      check(l.tile(0, 3, 2) == 0);
      l.layers.push_back({"Objects", std::vector<int>(12, 0)});
      l.paint(1, 2, 1, 7);
      check(deserialize(serialize(l)) == l);
      check(deserialize(serialize(l)).genre == genre);
      auto path = std::filesystem::temp_directory_path() /
                  ("pixelforge-test-" + genre_name(genre) + ".json");
      save(l, path);
      check(load(path) == l);
      auto replacement = l;
      replacement.paint(0, 0, 0, 19);
      save(replacement, path);
      check(load(path) == replacement);
      std::filesystem::remove(path);
      rejects([&] { l.paint(0, -1, 0, 1); });
      rejects([&] { l.paint(0, 4, 0, 1); });
      rejects([&] { l.paint(2, 0, 0, 1); });
      rejects([&] { l.paint(0, 0, 0, -1); });
      rejects([&] { l.paint(0, 0, 0, 256); });
    }
    rejects([] { pf::Level l(pf::Genre::platformer, 0, 1); });
    rejects([] { pf::Level l(pf::Genre::platformer, 257, 1); });
    rejects([] { deserialize("garbage"); });
    rejects([] { deserialize("{}"); });
    auto base =
        nlohmann::json::parse(serialize(Level(Genre::platformer, 2, 2)));
    auto invalid = [&](auto mutate) {
      auto j = base;
      mutate(j);
      rejects([&] { deserialize(j.dump()); });
    };
    invalid([](auto &j) { j["genre"] = "unknown"; });
    invalid([](auto &j) { j["version"] = 3; });
    invalid([](auto &j) { j["width"] = 1.5; });
    invalid([](auto &j) { j["width"] = 4294967297ULL; });
    invalid([](auto &j) { j["layers"] = nlohmann::json::array(); });
    invalid([](auto &j) { j["layers"][0]["tiles"] = {1, 2}; });
    invalid([](auto &j) { j["layers"][0]["tiles"][0] = -1; });
    invalid([](auto &j) { j["layers"][0]["tiles"][0] = 256; });
    invalid([](auto &j) { j["layers"][0]["tiles"][0] = 1.1; });
    invalid([](auto &j) { j["layers"][0]["name"] = ""; });
    invalid([](auto &j) { j["extra"] = true; });
    invalid([](auto &j) { j["collision"].erase(0); });
    invalid([](auto &j) { j["collision"][0] = 2; });
    invalid([](auto &j) { j["tile_size"]["width"] = 0; });
    invalid([](auto &j) { j["tileset"]["path"] = "../outside.png"; });
    invalid([](auto &j) { j["tileset"]["path"] = "assets/../outside.png"; });
    invalid([](auto &j) { j["tileset"]["path"] = "/tmp/outside.png"; });
    invalid([](auto &j) { j["tileset"]["path"] = "C:/outside.png"; });
    invalid([](auto &j) { j["tileset"]["path"] = R"(C:\outside.png)"; });
    invalid([](auto &j) {
      j["tileset"]["path"] = "//server/share/outside.png";
    });
    invalid([](auto &j) {
      j["tileset"]["path"] = R"(\\server\share\outside.png)";
    });
    auto relative_asset = base;
    relative_asset["tileset"]["path"] = "assets/custom/atlas.png";
    check(deserialize(relative_asset.dump()).tileset_path ==
          "assets/custom/atlas.png");
    invalid([](auto &j) {
      j["entities"].push_back({{"id", 1},
                               {"type", "npc"},
                               {"x", 1.0},
                               {"y", 1.0},
                               {"properties", nlohmann::json::object()}});
      j["entities"].push_back(j["entities"][0]);
    });
    invalid([](auto &j) {
      j["entities"].push_back({{"id", 1},
                               {"type", "npc"},
                               {"x", -1.0},
                               {"y", 1.0},
                               {"properties", nlohmann::json::object()}});
    });
    invalid([](auto &j) {
      j["entities"].push_back({{"id", 1},
                               {"type", "unknown"},
                               {"x", 1.0},
                               {"y", 1.0},
                               {"properties", nlohmann::json::object()}});
    });
    invalid([](auto &j) {
      j["entities"].push_back({{"id", 1},
                               {"type", "npc"},
                               {"x", 1.0},
                               {"y", 1.0},
                               {"properties", nlohmann::json::object()},
                               {"extra", true}});
    });
    auto legacy = base;
    legacy["version"] = 1;
    legacy.erase("tile_size");
    legacy.erase("tileset");
    legacy.erase("collision");
    legacy.erase("entities");
    legacy["layers"][0].erase("visible");
    legacy["layers"][0].erase("locked");
    const auto migrated = deserialize(legacy.dump());
    check(migrated.tile_width == 24 && migrated.tile_height == 24);
    check(migrated.layers[0].visible && !migrated.layers[0].locked);
    check(migrated.collision.size() == 4 && migrated.entities.empty());
    Level world(Genre::top_down_rpg, 3, 2);
    world.set_solid(1, 1, true);
    world.entities.push_back({7,
                              EntityType::npc,
                              1.5f,
                              0.5f,
                              {{"name", "Ada"}, {"dialogue", "Hello"}}});
    check(world.solid(1, 1) && world.solid(-1, 0) && !world.solid(0, 0));
    check(deserialize(serialize(world)) == world);
    rejects([&] { world.set_solid(3, 0, true); });
    rejects([] { load("missing-pixelforge-level.json"); });
    const auto failure_target =
        std::filesystem::temp_directory_path() / "pixelforge-save-failure";
    std::filesystem::remove_all(failure_target);
    std::filesystem::create_directory(failure_target);
    const auto marker = failure_target / "preserved.txt";
    {
      std::ofstream file(marker);
      file << "keep";
    }
    rejects([&] { save(Level(Genre::platformer, 2, 2), failure_target); });
    check(std::filesystem::is_directory(failure_target) &&
          std::filesystem::exists(marker));
    {
      std::ifstream file(marker);
      std::string contents;
      file >> contents;
      check(contents == "keep");
    }
    const auto temp_prefix = failure_target.filename().string() + ".tmp-";
    bool temporary_leaked = false;
    for (const auto &entry : std::filesystem::directory_iterator(
             failure_target.parent_path()))
      temporary_leaked = temporary_leaked ||
                         entry.path().filename().string().find(temp_prefix) == 0;
    check(!temporary_leaked);
    std::filesystem::remove_all(failure_target);
    rejects([] {
      Level l;
      l.layers[0].tiles.clear();
      serialize(l);
    });
    if (argc != 2)
      throw std::runtime_error("Example directory required");
    for (auto name : {"rpg.json", "platformer.json"}) {
      auto l = load(std::filesystem::path(argv[1]) / name);
      check(deserialize(serialize(l)) == l);
      check(genre_name(l.genre) ==
            (std::string(name) == "rpg.json" ? "top_down_rpg" : "platformer"));
    }
    std::cout << checks << " checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
