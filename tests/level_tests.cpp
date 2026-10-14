#include "serialization.hpp"
#include <filesystem>
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
    invalid([](auto &j) { j["version"] = 2; });
    invalid([](auto &j) { j["width"] = 1.5; });
    invalid([](auto &j) { j["width"] = 4294967297ULL; });
    invalid([](auto &j) { j["layers"] = nlohmann::json::array(); });
    invalid([](auto &j) { j["layers"][0]["tiles"] = {1, 2}; });
    invalid([](auto &j) { j["layers"][0]["tiles"][0] = -1; });
    invalid([](auto &j) { j["layers"][0]["tiles"][0] = 256; });
    invalid([](auto &j) { j["layers"][0]["tiles"][0] = 1.1; });
    invalid([](auto &j) { j["layers"][0]["name"] = ""; });
    rejects([] { load("missing-pixelforge-level.json"); });
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
