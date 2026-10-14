#include "gameplay.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pf::gameplay {
namespace {
constexpr float rpg_speed = 4.0f;
constexpr float platform_speed = 5.0f;
constexpr float gravity = 18.0f;
constexpr float jump_speed = 11.0f;
constexpr float interaction_distance = 0.85f;
float distance_squared(const Player &p, const Entity &e) {
  const float dx = p.x - e.x, dy = p.y - e.y;
  return dx * dx + dy * dy;
}
std::string property_string(const Entity &entity, const std::string &key,
                            const std::string &fallback) {
  const auto it = entity.properties.find(key);
  return it != entity.properties.end() && it->second.is_string()
             ? it->second.get<std::string>()
             : fallback;
}
bool overlaps(const Player &p, const Entity &e) {
  return std::abs(p.x - e.x) < 0.55f && std::abs(p.y - e.y) < 0.65f;
}
bool blocked(const World &world, float x, float y, float width, float height,
             int &tile_x, int &tile_y) {
  const int left = static_cast<int>(std::floor(x - width * 0.5f + 0.001f));
  const int right = static_cast<int>(std::floor(x + width * 0.5f - 0.001f));
  const int top = static_cast<int>(std::floor(y - height * 0.5f + 0.001f));
  const int bottom = static_cast<int>(std::floor(y + height * 0.5f - 0.001f));
  for (int cy = top; cy <= bottom; ++cy)
    for (int cx = left; cx <= right; ++cx)
      if (world.solid(cx, cy)) {
        tile_x = cx;
        tile_y = cy;
        return true;
      }
  return false;
}
void move_axis(const World &world, Player &player, float delta,
               bool horizontal) {
  const int steps =
      std::max(1, static_cast<int>(std::ceil(std::abs(delta) / 0.2f)));
  const float increment = delta / steps;
  for (int i = 0; i < steps; ++i) {
    const float x = player.x + (horizontal ? increment : 0);
    const float y = player.y + (horizontal ? 0 : increment);
    int tile_x = -1, tile_y = -1;
    if (blocked(world, x, y, player_hitbox_width, player_hitbox_height,
                tile_x, tile_y)) {
      if (horizontal) {
        const float half_width = player_hitbox_width * 0.5f;
        player.x = increment > 0 ? tile_x - half_width
                                 : tile_x + 1.0f + half_width;
      } else {
        const float half_height = player_hitbox_height * 0.5f;
        player.y = increment > 0 ? tile_y - half_height
                                 : tile_y + 1.0f + half_height;
      }
      if (!horizontal && increment > 0)
        player.grounded = true;
      if (horizontal)
        player.contact_normal_x = increment > 0 ? -1 : 1;
      else
        player.contact_normal_y = increment > 0 ? -1 : 1;
      player.contact_tile_x = tile_x;
      player.contact_tile_y = tile_y;
      if (horizontal) {
        player.velocity_x = 0;
      } else {
        player.velocity_y = 0;
        if (increment > 0) {
          player.support_tile_x = tile_x;
          player.support_tile_y = tile_y;
        }
      }
      break;
    }
    player.x = x;
    player.y = y;
    if (!horizontal && increment < 0)
      player.grounded = false;
  }
}
void respawn(World &world, Player &player) {
  player.x = world.checkpoint_x;
  player.y = world.checkpoint_y;
  player.velocity_x = player.velocity_y = 0;
  player.grounded = false;
  player.contact_normal_x = player.contact_normal_y = 0;
  player.contact_tile_x = player.contact_tile_y = -1;
  player.support_tile_x = player.support_tile_y = -1;
}
void collect_touching(World &world, const Player &player,
                      std::string &message) {
  auto &entities = world.entities;
  for (auto it = entities.begin(); it != entities.end();) {
    if (it->type == EntityType::collectible && overlaps(player, *it)) {
      message = "Collected " + property_string(*it, "name", "item");
      it = entities.erase(it);
    } else
      ++it;
  }
}
void resolve_contacts(World &world, Player &player, std::string &message) {
  for (const auto &entity : world.entities) {
    if (entity.type == EntityType::checkpoint && overlaps(player, entity)) {
      world.checkpoint_x = entity.x;
      world.checkpoint_y = entity.y - 0.5f;
      message = "Checkpoint reached";
    } else if (entity.type == EntityType::hazard && overlaps(player, entity)) {
      respawn(world, player);
      message = "Hit a hazard; respawned";
      return;
    } else if (entity.type == EntityType::trigger && overlaps(player, entity)) {
      const auto name = property_string(entity, "name", "Area");
      message = "Reached " + name;
    }
  }
  collect_touching(world, player, message);
}
} // namespace
Bounds bounds(const Player &player) {
  const float half_width = player_hitbox_width * 0.5f;
  const float half_height = player_hitbox_height * 0.5f;
  return {player.x - half_width, player.y - half_height,
          player.x + half_width, player.y + half_height};
}
bool World::solid(int x, int y) const {
  if (x < 0 || y < 0 || x >= width)
    return true;
  if (y >= height)
    return genre != Genre::platformer;
  return collision.at(static_cast<std::size_t>(y * width + x)) != 0;
}
RuntimeLevel make_runtime_level(const Level &level) {
  level.validate();
  RuntimeLevel result{level, {}};
  result.world.width = level.width;
  result.world.height = level.height;
  result.world.genre = level.genre;
  result.world.collision = level.collision;
  result.world.entities = level.entities;
  const auto spawn = std::find_if(
      result.world.entities.begin(), result.world.entities.end(),
      [](const auto &e) { return e.type == EntityType::player_spawn; });
  if (spawn != result.world.entities.end()) {
    result.world.checkpoint_x = spawn->x;
    result.world.checkpoint_y = spawn->y;
  }
  return result;
}
void TopDownController::step(World &world, Player &player, const Input &input,
                             float dt, std::string &message) {
  player.grounded = false;
  player.contact_normal_x = player.contact_normal_y = 0;
  player.contact_tile_x = player.contact_tile_y = -1;
  player.support_tile_x = player.support_tile_y = -1;
  float dx = static_cast<float>(input.right) - static_cast<float>(input.left);
  float dy = static_cast<float>(input.down) - static_cast<float>(input.up);
  const float length = std::sqrt(dx * dx + dy * dy);
  if (length > 0) {
    dx /= length;
    dy /= length;
  }
  move_axis(world, player, dx * rpg_speed * dt, true);
  move_axis(world, player, dy * rpg_speed * dt, false);
  if (input.interact) {
    const Entity *nearest = nullptr;
    float closest = interaction_distance * interaction_distance;
    for (const auto &entity : world.entities)
      if (entity.type == EntityType::npc) {
        const float distance = distance_squared(player, entity);
        if (distance <= closest) {
          closest = distance;
          nearest = &entity;
        }
      }
    if (nearest)
      message = property_string(*nearest, "message",
                                property_string(*nearest, "name", "Hello!"));
  }
  resolve_contacts(world, player, message);
}
void PlatformerController::step(World &world, Player &player,
                                const Input &input, float dt,
                                std::string &message) {
  player.contact_normal_x = player.contact_normal_y = 0;
  player.contact_tile_x = player.contact_tile_y = -1;
  player.support_tile_x = player.support_tile_y = -1;
  player.velocity_x =
      (static_cast<float>(input.right) - static_cast<float>(input.left)) *
      platform_speed;
  if (input.jump && player.grounded) {
    player.velocity_y = -jump_speed;
    player.grounded = false;
  }
  player.velocity_y = std::min(player.velocity_y + gravity * dt, 18.0f);
  move_axis(world, player, player.velocity_x * dt, true);
  player.grounded = false;
  move_axis(world, player, player.velocity_y * dt, false);
  if (player.y - player_hitbox_height * 0.5f > world.height) {
    respawn(world, player);
    message = "Fell; respawned";
  } else
    resolve_contacts(world, player, message);
}
Session::Session(const Level &level) : initial_(make_runtime_level(level)) {
  if (level.genre == Genre::top_down_rpg)
    controller_ = std::make_unique<TopDownController>();
  else if (level.genre == Genre::platformer)
    controller_ = std::make_unique<PlatformerController>();
  else
    throw std::invalid_argument("Unsupported runtime genre");
  reset_world();
}
void Session::reset_world() {
  world_ = initial_.world;
  player_ = {};
  player_.x = world_.checkpoint_x;
  player_.y = world_.checkpoint_y;
  accumulator_ = 0;
  message_.clear();
}
void Session::advance(float elapsed, const Input &input) {
  if (paused_)
    return;
  accumulator_ += std::clamp(elapsed, 0.0f, 0.25f);
  while (accumulator_ >= fixed_step) {
    step(input);
    accumulator_ -= fixed_step;
  }
}
void Session::step(const Input &input) {
  if (!paused_)
    controller_->step(world_, player_, input, fixed_step, message_);
}
void Session::reset() {
  paused_ = false;
  reset_world();
}
} // namespace pf::gameplay
