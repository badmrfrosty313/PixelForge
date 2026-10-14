#include "document.hpp"
#include "serialization.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
int checks = 0;
void check(bool v) {
  ++checks;
  if (!v)
    throw std::runtime_error("check " + std::to_string(checks) + " failed");
}
template <class F> void rejects(F f) {
  bool failed = false;
  try {
    f();
  } catch (const std::exception &) {
    failed = true;
  }
  check(failed);
}
} // namespace
int main() {
  try {
    using namespace pf;
    Document d(Level(Genre::top_down_rpg, 12, 10));
    d.mark_saved();
    check(!d.dirty());
    d.begin_stroke();
    d.paint_line(0, 1, 1, 5, 1, 4);
    d.paint_line(0, 5, 1, 8, 4, 4);
    d.end_stroke();
    check(d.dirty() && d.can_undo() && !d.can_redo());
    for (int x = 1; x <= 8; ++x)
      check(d.tile(0, x, 1 + (x > 5 ? x - 5 : 0)) == 4);
    d.mark_saved();
    d.undo();
    check(d.tile(0, 1, 1) == 0 && d.tile(0, 6, 2) == 0 && d.dirty());
    d.redo();
    check(d.tile(0, 8, 4) == 4 && !d.dirty());
    d.undo();
    d.paint(0, 2, 2, 7);
    check(!d.can_redo());
    d.undo();
    check(d.tile(0, 1, 1) == 0);
    const auto before = d.level();
    d.begin_stroke();
    d.paint_line(0, 3, 3, 3, 3, 0);
    d.end_stroke();
    check(d.level() == before && !d.can_undo());
    d.line(0, 0, 0, 5, 5, 2);
    check(d.tile(0, 4, 4) == 2);
    d.undo();
    check(d.tile(0, 4, 4) == 0);
    d.redo();
    check(d.tile(0, 5, 5) == 2);
    d.rectangle(0, 2, 2, 4, 4, 3);
    for (int y = 2; y <= 4; ++y)
      for (int x = 2; x <= 4; ++x)
        check(d.tile(0, x, y) == 3);
    d.undo();
    check(d.tile(0, 3, 3) == 2);
    Document fill(Level(Genre::platformer, 5, 4));
    fill.flood_fill(0, 0, 0, 9);
    check(fill.tile(0, 4, 3) == 9);
    fill.undo();
    check(fill.tile(0, 4, 3) == 0);
    fill.redo();
    check(fill.tile(0, 2, 2) == 9);
    const auto filled = fill.level();
    const bool was_dirty = fill.dirty();
    fill.flood_fill(0, 0, 0, 9);
    check(fill.level() == filled && fill.dirty() == was_dirty);
    rejects([&] { fill.flood_fill(0, 0, 0, 300); });
    Document world(Level(Genre::top_down_rpg, 6, 5));
    world.begin_collision_stroke();
    world.paint_collision_line(0, 2, 5, 2, true);
    world.paint_collision_line(5, 2, 5, 4, true);
    world.end_collision_stroke();
    check(world.solid(0, 2) && world.solid(5, 4));
    world.undo();
    check(!world.solid(0, 2) && !world.solid(5, 4));
    world.redo();
    check(world.solid(3, 2));
    const auto npc_id = world.add_entity(EntityType::npc, 2.5f, 1.5f);
    Entity npc = world.level().entities.front();
    npc.properties["name"] = "Guide";
    npc.properties["message"] = "Go north";
    world.update_entity(npc);
    check(world.level().entities[0].id == npc_id &&
          world.level().entities[0].properties.at("name") == "Guide");
    world.undo();
    check(world.level().entities[0].properties.empty());
    world.redo();
    world.delete_entity(npc_id);
    check(world.level().entities.empty());
    world.undo();
    check(world.level().entities.size() == 1);
    rejects([&] { world.add_entity(EntityType::hazard, -1, 0); });
    const auto hazard_id = world.add_entity(EntityType::hazard, 1, 1);
    check(hazard_id != npc_id);
    const auto saved_world = world.level();
    world.set_asset_settings("assets/other.png", 32, 32, 8, 8);
    check(world.level().tile_width == 32 && world.dirty());
    world.undo();
    check(world.level().tileset_path == saved_world.tileset_path &&
          world.level().tile_width == 24);
    world.redo();
    check(world.level().atlas_tile_width == 8);
    Document layers(Level(Genre::top_down_rpg, 3, 2));
    layers.mark_saved();
    layers.create_layer("Details");
    check(layers.level().layers.size() == 2 && layers.dirty());
    layers.rename_layer(1, "Decals");
    layers.move_layer(1, 0);
    check(layers.level().layers[0].name == "Decals");
    layers.delete_layer(1);
    check(layers.level().layers.size() == 1);
    layers.undo();
    check(layers.level().layers.size() == 2);
    layers.undo();
    check(layers.level().layers[0].name == "Ground");
    layers.undo();
    check(layers.level().layers[1].name == "Details");
    layers.undo();
    check(layers.level().layers.size() == 1 && !layers.dirty());
    rejects([&] { layers.delete_layer(0); });
    rejects([&] { layers.create_layer(""); });
    check(layers.level().layers.size() == 1);
    layers.create_layer("Overlay");
    layers.set_layer_visible(1, false);
    layers.set_layer_locked(1, true);
    check(!layers.level().layers[1].visible && layers.level().layers[1].locked);
    const auto locked = layers.level();
    layers.paint(1, 0, 0, 8);
    layers.rectangle(1, 0, 0, 2, 1, 9);
    layers.flood_fill(1, 0, 0, 7);
    check(layers.level() == locked);
    const auto roundtrip = deserialize(serialize(layers.level()));
    check(roundtrip == layers.level());
    auto legacy = serialize(Level{});
    auto j = nlohmann::json::parse(legacy);
    j["version"] = 1;
    j.erase("tile_size");
    j.erase("tileset");
    j.erase("collision");
    j.erase("entities");
    for (auto &l : j["layers"]) {
      l.erase("visible");
      l.erase("locked");
    }
    auto old = deserialize(j.dump());
    check(old.layers[0].visible && !old.layers[0].locked);
    const auto saved = layers.level();
    layers.paint(1, 0, 0, 5);
    check(layers.tile(1, 0, 0) == 0);
    check(layers.level() == saved);
    Document bounded(Level(Genre::platformer, 1, 1));
    for (int i = 1; i <= 80; ++i)
      bounded.paint(0, 0, 0, i);
    for (int i = 0; i < 64; ++i)
      bounded.undo();
    check(bounded.tile(0, 0, 0) == 16 && !bounded.can_undo());
    std::cout << checks << " document checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
