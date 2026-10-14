#include "gameplay.hpp"
#include "serialization.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace pf;
using namespace pf::gameplay;
namespace {
int checks = 0;
void check(bool condition, const char *message) {
  ++checks;
  if (!condition)
    throw std::runtime_error(message);
}
void close(float actual, float expected, float epsilon, const char *message) {
  check(std::abs(actual - expected) <= epsilon, message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    Level rpg(Genre::top_down_rpg, 10, 8);
    rpg.entities = {
        {1, EntityType::player_spawn, 2.5f, 2.5f, {}},
        {2, EntityType::npc, 3.2f, 2.5f, {{"message", "North path"}}},
        {3, EntityType::collectible, 5.5f, 2.5f, {{"name", "Gem"}}}};
    rpg.set_solid(4, 2, true);
    Session topdown(rpg);
    check(topdown.player().x == 2.5f && topdown.player().y == 2.5f,
          "spawn resolves from entity");
    Input move_right;
    move_right.right = true;
    for (int i = 0; i < 100; ++i)
      topdown.step(move_right);
    check(topdown.player().x < 3.8f, "top-down collision blocks solid tile");
    Input interact;
    interact.interact = true;
    topdown.step(interact);
    check(topdown.message() == "North path", "NPC interaction shows message");
    const float before_pause = topdown.player().x;
    topdown.set_paused(true);
    topdown.advance(1.0f, move_right);
    close(topdown.player().x, before_pause, 0.001f, "pause freezes simulation");
    topdown.set_paused(false);
    topdown.reset();
    check(topdown.world().entities.size() == 3,
          "reset restores collected world state");
    close(topdown.player().x, 2.5f, 0.001f, "reset restores spawn");
    Level collection_level(Genre::top_down_rpg, 8, 6);
    collection_level.entities = {
        {1, EntityType::player_spawn, 2.5f, 2.5f, {}},
        {2, EntityType::collectible, 3.2f, 2.5f, {{"name", "Gem"}}}};
    Session collection_session(collection_level);
    for (int i = 0; i < 10; ++i)
      collection_session.step(move_right);
    check(collection_session.message() == "Collected Gem",
          "touching collectible triggers and removes it");
    check(collection_session.world().entities.size() == 1 &&
              collection_level.entities.size() == 2,
          "collection is runtime-only and preserves saved level data");
    check(rpg.entities.size() == 3 && rpg.tile(0, 5, 2) == 0,
          "playtest cannot modify saved level data");

    Level platform(Genre::platformer, 10, 8);
    for (int x = 0; x < platform.width; ++x)
      platform.set_solid(x, 6, true);
    platform.entities = {{1, EntityType::player_spawn, 1.5f, 5.0f, {}},
                         {2, EntityType::checkpoint, 3.5f, 5.5f, {}},
                         {3, EntityType::hazard, 5.5f, 5.5f, {}}};
    Session platformer(platform);
    for (int i = 0; i < 90; ++i)
      platformer.step({});
    check(platformer.player().grounded, "gravity settles player on ground");
    close(platformer.player().y, 5.61f, 0.03f, "ground collision stabilizes y");
    const float grounded_y = platformer.player().y;
    Input jump;
    jump.jump = true;
    platformer.step(jump);
    check(platformer.player().y < grounded_y &&
              platformer.player().velocity_y < 0,
          "jump applies upward velocity");
    for (int i = 0; i < 120; ++i)
      platformer.step({});
    check(platformer.player().grounded, "player lands after jump");
    Input run;
    run.right = true;
    bool checkpoint_seen = false, hazard_respawned = false;
    for (int i = 0; i < 120; ++i) {
      platformer.step(run);
      checkpoint_seen =
          checkpoint_seen || platformer.message() == "Checkpoint reached";
      hazard_respawned =
          hazard_respawned || platformer.message() == "Hit a hazard; respawned";
      if (hazard_respawned)
        break;
    }
    check(checkpoint_seen, "checkpoint updates respawn state");
    check(hazard_respawned, "hazard respawns the player");
    close(platformer.player().x, 3.5f, 0.01f,
          "hazard respawns at most recent checkpoint");
    const auto original_entities = platform.entities.size();
    platformer.reset();
    check(platformer.world().entities.size() == original_entities,
          "repeated session reset restores entities");

    World bounds{2, 2, std::vector<std::uint8_t>(4, 0), {}, 0.5f, 0.5f};
    check(bounds.solid(-1, 0) && bounds.solid(2, 1), "world bounds are solid");

    if (argc != 2)
      throw std::runtime_error("Example directory required");
    const std::filesystem::path examples = argv[1];
    auto repeat = [&](Session &session, const Input &input, int count) {
      for (int i = 0; i < count; ++i)
        session.step(input);
    };
    Session rpg_demo(load(examples / "rpg.json"));
    Input down;
    down.down = true;
    repeat(rpg_demo, move_right, 45);
    repeat(rpg_demo, down, 45);
    rpg_demo.step(interact);
    check(rpg_demo.message().find("Find the crystal") != std::string::npos,
          "RPG sample NPC is reachable and interactive");
    rpg_demo.reset();
    repeat(rpg_demo, down, 105);
    repeat(rpg_demo, move_right, 180);
    Input up;
    up.up = true;
    repeat(rpg_demo, up, 60);
    check(rpg_demo.message() == "Collected Moon crystal",
          "RPG sample collectible is reachable around the wall");

    Session platform_demo(load(examples / "platformer.json"));
    repeat(platform_demo, Input{}, 8);
    repeat(platform_demo, move_right, 18);
    Input sample_jump;
    sample_jump.jump = true;
    platform_demo.step(sample_jump);
    repeat(platform_demo, Input{}, 25);
    repeat(platform_demo, move_right, 45);
    repeat(platform_demo, Input{}, 20);
    check(platform_demo.player().grounded && platform_demo.player().x < 9,
          "platformer sample first jump reaches the lower platform");
    repeat(platform_demo, move_right, 39);
    platform_demo.step(sample_jump);
    repeat(platform_demo, Input{}, 25);
    repeat(platform_demo, move_right, 50);
    repeat(platform_demo, Input{}, 12);
    check(platform_demo.player().grounded && platform_demo.player().x >= 12.5f,
          "platformer sample second jump reaches the upper platform");
    repeat(platform_demo, move_right, 8);
    check(platform_demo.world().checkpoint_x > 14,
          "platformer sample checkpoint is reachable");
    repeat(platform_demo, move_right, 45);
    repeat(platform_demo, Input{}, 50);
    check(platform_demo.message().find("Reached Finish") != std::string::npos,
          "platformer sample finish trigger is reachable");
    std::cout << checks << " gameplay checks passed\n";
  } catch (const std::exception &error) {
    std::cerr << "Gameplay test failure after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
