#include "gameplay.hpp"
#include "coordinates.hpp"
#include "serialization.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace pf;
using namespace pf::gameplay;
namespace {
int checks = 0;
nlohmann::json check_report = nlohmann::json::array();
void check(bool condition, const char *message) {
  ++checks;
  check_report.push_back({{"assertion", message}, {"passed", condition}});
  if (!condition)
    throw std::runtime_error(message);
}
void close(float actual, float expected, float epsilon, const char *message) {
  check(std::abs(actual - expected) <= epsilon, message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    using namespace pf;
    using namespace pf::gameplay;
    auto repeat = [&](Session &session, const Input &input, int count) {
      for (int i = 0; i < count; ++i)
        session.step(input);
    };
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
    check(topdown.player().moving && topdown.player().facing_x == 1,
          "top-down movement updates the character facing state");
    Input move_up;
    move_up.up = move_up.right = true;
    topdown.step(move_up);
    check(topdown.player().facing_y == -1,
          "top-down movement records the character vertical facing");
    check(topdown.player().x < 3.8f, "top-down collision blocks solid tile");
    close(topdown.player().x, 4.0f - player_hitbox_width * 0.5f, 0.001f,
          "top-down wall contact resolves to the exact hitbox face");
    check(topdown.player().contact_normal_x == -1 &&
              topdown.player().contact_tile_x == 4 &&
              topdown.player().velocity_x == 0,
          "top-down wall collision reports its tile and normal");
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

    auto make_floor = [](int width, int height, int row) {
      Level level(Genre::platformer, width, height);
      for (int x = 0; x < width; ++x)
        level.set_solid(x, row, true);
      return level;
    };
    auto settle = [](Session &session, int frames = 180) {
      for (int i = 0; i < frames; ++i)
        session.step({});
    };

    auto flat = make_floor(12, 12, 8);
    flat.entities = {{1, EntityType::player_spawn, 2.5f, 2.5f, {}}};
    Session flat_floor(flat);
    settle(flat_floor);
    check(flat_floor.player().grounded, "flat-floor landing sets grounded");
    const auto flat_bounds = pf::gameplay::bounds(flat_floor.player());
    close(flat_bounds.bottom, 8.0f, 0.005f,
          "collision-box feet rest exactly on the floor top");
    check(flat_floor.player().contact_tile_x == 2 &&
              flat_floor.player().contact_tile_y == 8 &&
              flat_floor.player().support_tile_x == 2 &&
              flat_floor.player().support_tile_y == 8 &&
              flat_floor.player().contact_normal_x == 0 &&
              flat_floor.player().contact_normal_y == -1,
          "ground contact reports the support tile and upward normal");
    const pf::coordinates::TileCamera camera{120.0f, 80.0f, 24.0f};
    const auto screen_player = camera.project(
        flat_bounds.left, flat_bounds.top, flat_bounds.right,
        flat_bounds.bottom);
    const auto screen_floor = camera.project(2.5f, 8.0f);
    close(screen_player.bottom, screen_floor.y, 0.001f,
          "camera projection keeps rendered feet flush with support tile");
    close(screen_player.left, camera.project(flat_bounds.left, 0).x, 0.001f,
          "player hitbox uses the shared tile-to-screen camera scale");
    const auto round_trip = camera.unproject(screen_floor.x, screen_floor.y);
    close(round_trip.x, 2.5f, 0.001f,
          "camera inverse recovers horizontal world position");
    close(round_trip.y, 8.0f, 0.001f,
          "camera inverse recovers vertical world position");
    const pf::coordinates::TileGridTransform grid{38.0f, 72.0f, 30.0f, 18.0f};
    const auto grid_screen = grid.project(4.25f, 7.5f);
    const auto grid_tile = grid.unproject(grid_screen.x, grid_screen.y);
    close(grid_tile.x, 4.25f, 0.001f,
          "mouse-to-tile transform round-trips non-square tile width");
    close(grid_tile.y, 7.5f, 0.001f,
          "mouse-to-tile transform round-trips non-square tile height");

    auto elevated = make_floor(14, 12, 9);
    for (int x = 4; x <= 8; ++x)
      elevated.set_solid(x, 5, true);
    elevated.entities = {{1, EntityType::player_spawn, 6.5f, 2.0f, {}}};
    Session raised_platform(elevated);
    settle(raised_platform);
    check(raised_platform.player().grounded &&
              raised_platform.player().contact_tile_y == 5,
          "player lands on elevated platform rather than the distant floor");
    close(pf::gameplay::bounds(raised_platform.player()).bottom, 5.0f,
          0.005f, "elevated platform landing is flush");
    const float raised_y = raised_platform.player().y;
    Input one_jump;
    one_jump.jump = true;
    raised_platform.step(one_jump);
    check(raised_platform.player().y < raised_y &&
              raised_platform.player().velocity_y < 0,
          "jump begins from the grounded center position");
    for (int i = 0; i < 120; ++i)
      raised_platform.step({});
    check(raised_platform.player().grounded,
          "jump returns to a stable grounded state");
    for (int jump_index = 0; jump_index < 8; ++jump_index) {
      raised_platform.step(one_jump);
      for (int i = 0; i < 120; ++i)
        raised_platform.step({});
      close(pf::gameplay::bounds(raised_platform.player()).bottom, 5.0f,
            0.005f, "repeated jumps do not accumulate vertical drift");
    }

    auto ledge = Level(Genre::platformer, 12, 12);
    for (int x = 0; x <= 2; ++x)
      ledge.set_solid(x, 8, true);
    ledge.entities = {{1, EntityType::player_spawn, 2.5f, 7.61f, {}}};
    Session walking_off(ledge);
    settle(walking_off, 10);
    Input walk_right;
    walk_right.right = true;
    Level beside_platform_level(Genre::platformer, 12, 12);
    for (int x = 0; x <= 3; ++x)
      beside_platform_level.set_solid(x, 8, true);
    for (int x = 4; x <= 7; ++x)
      beside_platform_level.set_solid(x, 6, true);
    beside_platform_level.entities = {
        {1, EntityType::player_spawn, 3.3f, 8.61f, {}}};
    Session beside_platform(beside_platform_level);
    settle(beside_platform, 10);
    beside_platform.step(one_jump);
    settle(beside_platform, 15);
    repeat(beside_platform, walk_right, 12);
    settle(beside_platform, 90);
    check(beside_platform.player().grounded &&
              beside_platform.player().contact_tile_y == 6 &&
              beside_platform.player().x >= 4.0f,
          "jumping beside a raised platform reaches its walkable top");
    for (int i = 0; i < 30; ++i)
      walking_off.step(walk_right);
    check(!walking_off.player().grounded &&
              walking_off.player().velocity_y > 0,
          "walking off a ledge transitions from grounded to falling");

    auto wall_level = make_floor(12, 12, 9);
    for (int y = 1; y < 9; ++y)
      wall_level.set_solid(4, y, true);
    wall_level.entities = {{1, EntityType::player_spawn, 2.5f, 8.61f, {}}};
    Session wall_collision(wall_level);
    settle(wall_collision, 10);
    for (int i = 0; i < 100; ++i)
      wall_collision.step(walk_right);
    check(wall_collision.player().x <= 4.0f - 0.31f + 0.01f,
          "horizontal wall collision blocks the player hitbox");
    check(wall_collision.player().grounded,
          "horizontal movement against a wall preserves floor grounding");
    check(wall_collision.player().contact_normal_x == -1 &&
              wall_collision.player().contact_normal_y == -1 &&
              wall_collision.player().support_tile_y == 9,
          "debug contacts retain wall and floor normals together");
    close(wall_collision.player().y, 9.0f - player_hitbox_height * 0.5f,
          0.001f,
          "horizontal wall movement does not perturb the grounded height");

    auto ceiling_level = make_floor(12, 14, 11);
    for (int x = 2; x <= 4; ++x)
      ceiling_level.set_solid(x, 7, true);
    ceiling_level.entities = {{1, EntityType::player_spawn, 3.5f, 10.61f, {}}};
    Session ceiling(ceiling_level);
    settle(ceiling, 10);
    bool ceiling_hit = false;
    float ceiling_contact_y = 0;
    ceiling.step(one_jump);
    for (int i = 0; i < 100; ++i) {
      ceiling.step({});
      if (ceiling.player().contact_normal_y == 1) {
        ceiling_hit = true;
        ceiling_contact_y = ceiling.player().y;
      }
    }
    check(ceiling_hit, "jumping into a ceiling reports an opposing normal");
    close(ceiling_contact_y, 8.0f + player_hitbox_height * 0.5f, 0.001f,
          "ceiling collision snaps the player to the underside");

    auto high_fall_level = make_floor(8, 40, 35);
    high_fall_level.entities = {
        {1, EntityType::player_spawn, 3.5f, 1.5f, {}}};
    Session high_fall(high_fall_level);
    settle(high_fall, 300);
    check(high_fall.player().grounded &&
              high_fall.player().contact_tile_y == 35,
          "high-velocity fall lands without tunneling through the floor");

    Level fall_out_level(Genre::platformer, 8, 10);
    fall_out_level.entities = {{1, EntityType::player_spawn, 3.5f, 2.5f, {}}};
    Session fall_out(fall_out_level);
    bool fell_out = false;
    for (int i = 0; i < 180; ++i) {
      fall_out.step({});
      if (fall_out.message() == "Fell; respawned") {
        fell_out = true;
        break;
      }
    }
    check(fell_out, "falling below a platformer map reaches fall respawn");
    close(fall_out.player().x, 3.5f, 0.001f,
          "falling below the map restores the spawn x coordinate");
    close(fall_out.player().y, 2.5f, 0.001f,
          "falling below the map restores the spawn y coordinate");
    World rpg_bounds;
    rpg_bounds.width = 2;
    rpg_bounds.height = 2;
    check(rpg_bounds.solid(0, 2), "top-down worlds retain solid lower bounds");

    auto corner_level = make_floor(12, 12, 8);
    for (int y = 0; y < 8; ++y)
      corner_level.set_solid(5, y, true);
    corner_level.entities = {{1, EntityType::player_spawn, 2.5f, 7.61f, {}}};
    Session corner(corner_level);
    settle(corner, 10);
    for (int i = 0; i < 600; ++i)
      corner.step(walk_right);
    const float corner_x = corner.player().x;
    const float corner_y = corner.player().y;
    for (int i = 0; i < 600; ++i)
      corner.step(walk_right);
    close(corner.player().x, corner_x, 0.001f,
          "repeated corner contact remains horizontally stable");
    close(corner.player().y, corner_y, 0.001f,
          "repeated corner contact remains vertically stable");

    Level open_rpg(Genre::top_down_rpg, 40, 40);
    open_rpg.entities = {{1, EntityType::player_spawn, 20.5f, 20.5f, {}}};
    Session cardinal(open_rpg);
    repeat(cardinal, move_right, 60);
    const float cardinal_distance = cardinal.player().x - 20.5f;
    Session diagonal(open_rpg);
    Input diagonal_input;
    diagonal_input.right = diagonal_input.down = true;
    repeat(diagonal, diagonal_input, 60);
    const float diagonal_x = diagonal.player().x - 20.5f;
    const float diagonal_y = diagonal.player().y - 20.5f;
    close(std::sqrt(diagonal_x * diagonal_x + diagonal_y * diagonal_y),
          cardinal_distance, 0.01f,
          "RPG diagonal movement has normalized cardinal speed");

    Level npc_level(Genre::top_down_rpg, 10, 6);
    npc_level.entities = {{1, EntityType::player_spawn, 2.5f, 2.5f, {}},
                          {2, EntityType::npc, 3.5f, 2.5f, {}}};
    Session pass_through_npc(npc_level);
    repeat(pass_through_npc, move_right, 20);
    check(pass_through_npc.player().x > 3.0f,
          "RPG entities do not block movement unless marked solid");

    for (int reset_index = 0; reset_index < 12; ++reset_index) {
      wall_collision.reset();
      settle(wall_collision, 180);
      check(wall_collision.player().grounded &&
                wall_collision.player().x == 2.5f,
            "repeated session reset restores a valid grounded spawn");
    }

    if (argc < 2 || argc > 3)
      throw std::runtime_error("Example directory required");
    const std::filesystem::path examples = argv[1];
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
    if (argc >= 3) {
      std::ofstream report(argv[2]);
      report << nlohmann::json{{"schema_version", 1}, {"suite", "deterministic-demo-gameplay"}, {"checks", check_report}, {"passed", true}}.dump(2) << '\n';
      if (!report) throw std::runtime_error("Could not write gameplay report");
    }
    std::cout << checks << " gameplay checks passed\n";
  } catch (const std::exception &error) {
    std::cerr << "Gameplay test failure after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
