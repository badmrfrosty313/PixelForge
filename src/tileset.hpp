#pragma once
#include <SDL3/SDL.h>
#include <filesystem>
#include <memory>
namespace pf {
struct TextureRegion {
  float u0, v0, u1, v1;
};
class Tileset {
  struct TextureDeleter {
    void operator()(SDL_Texture *texture) const noexcept;
  };
  std::unique_ptr<SDL_Texture, TextureDeleter> texture_;
  int width_ = 0, height_ = 0, tile_width_ = 16, tile_height_ = 16;
  std::filesystem::path path_;

public:
  void load(SDL_Renderer *renderer, const std::filesystem::path &path,
            int tile_width, int tile_height);
  bool loaded() const noexcept { return static_cast<bool>(texture_); }
  int tile_count() const noexcept {
    return loaded() ? (width_ / tile_width_) * (height_ / tile_height_) : 0;
  }
  int tile_width() const noexcept { return tile_width_; }
  int tile_height() const noexcept { return tile_height_; }
  const std::filesystem::path &path() const noexcept { return path_; }
  SDL_Texture *texture() const noexcept { return texture_.get(); }
  TextureRegion region(int id) const;
};
} // namespace pf
