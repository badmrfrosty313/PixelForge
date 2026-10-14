#pragma once
#include "level.hpp"
#include <memory>
#include <string>
#include <vector>
namespace pf::gameplay {
struct Input {
  bool left = false, right = false, up = false, down = false, jump = false,
       interact = false;
};
struct Player {
  float x = 0, y = 0, velocity_x = 0, velocity_y = 0;
  bool grounded = false;
};
struct World {
  int width = 0, height = 0;
  std::vector<std::uint8_t> collision;
  std::vector<Entity> entities;
  float checkpoint_x = 0.5f, checkpoint_y = 0.5f;
  bool solid(int x, int y) const;
};
struct RuntimeLevel {
  Level visual;
  World world;
};
RuntimeLevel make_runtime_level(const Level &level);
class Controller {
public:
  virtual ~Controller() = default;
  virtual void step(World &world, Player &player, const Input &input, float dt,
                    std::string &message) = 0;
};
class TopDownController final : public Controller {
public:
  void step(World &, Player &, const Input &, float, std::string &) override;
};
class PlatformerController final : public Controller {
public:
  void step(World &, Player &, const Input &, float, std::string &) override;
};
class Session {
  RuntimeLevel initial_;
  World world_;
  Player player_;
  std::unique_ptr<Controller> controller_;
  float accumulator_ = 0;
  bool paused_ = false;
  std::string message_;
  void reset_world();

public:
  static constexpr float fixed_step = 1.0f / 60.0f;
  explicit Session(const Level &level);
  void advance(float elapsed, const Input &input);
  void step(const Input &input);
  void reset();
  void set_paused(bool paused) { paused_ = paused; }
  bool paused() const { return paused_; }
  const Level &visual_level() const { return initial_.visual; }
  const World &world() const { return world_; }
  const Player &player() const { return player_; }
  const std::string &message() const { return message_; }
};
} // namespace pf::gameplay
