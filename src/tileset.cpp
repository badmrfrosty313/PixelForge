#include "tileset.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>
#include <string>
namespace pf {
void Tileset::TextureDeleter::operator()(SDL_Texture *t) const noexcept {
  if (t)
    SDL_DestroyTexture(t);
}
void Tileset::load(SDL_Renderer *renderer, const std::filesystem::path &path,
                   int tile_width, int tile_height) {
  if (!renderer)
    throw std::invalid_argument("Renderer is unavailable");
  if (tile_width < 1 || tile_height < 1 || tile_width > 512 ||
      tile_height > 512)
    throw std::invalid_argument("Tile dimensions must be 1..512 pixels");
  int w = 0, h = 0, channels = 0;
  const auto file = path.string();
  if (!stbi_info(file.c_str(), &w, &h, &channels))
    throw std::runtime_error("Cannot read tileset PNG '" + file + "': " +
                             (stbi_failure_reason() ? stbi_failure_reason()
                                                    : "invalid image header"));
  if (w <= 0 || h <= 0 || w > 8192 || h > 8192 ||
      static_cast<std::uint64_t>(w) * static_cast<std::uint64_t>(h) >
          16 * 1024 * 1024)
    throw std::runtime_error(
        "Tileset image exceeds the 8192-pixel / 16-megapixel limit");
  stbi_uc *decoded = stbi_load(file.c_str(), &w, &h, &channels, STBI_rgb_alpha);
  if (!decoded)
    throw std::runtime_error(
        "Cannot load tileset '" + file + "': " +
        (stbi_failure_reason() ? stbi_failure_reason() : "invalid image"));
  struct Pixels {
    stbi_uc *value;
    ~Pixels() { stbi_image_free(value); }
  } pixels{decoded};
  if (w <= 0 || h <= 0 || w % tile_width != 0 || h % tile_height != 0)
    throw std::runtime_error(
        "Tileset dimensions must be divisible by the configured tile size");
  SDL_Surface *surface =
      SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, decoded, w * 4);
  if (!surface)
    throw std::runtime_error(std::string("Cannot create tileset surface: ") +
                             SDL_GetError());
  struct Surface {
    SDL_Surface *value;
    ~Surface() { SDL_DestroySurface(value); }
  } owned_surface{surface};
  std::unique_ptr<SDL_Texture, TextureDeleter> next(
      SDL_CreateTextureFromSurface(renderer, surface));
  if (!next)
    throw std::runtime_error(std::string("Cannot create tileset texture: ") +
                             SDL_GetError());
  if (!SDL_SetTextureScaleMode(next.get(), SDL_SCALEMODE_NEAREST))
    throw std::runtime_error(std::string("Cannot set pixel-art filtering: ") +
                             SDL_GetError());
  if (!SDL_SetTextureBlendMode(next.get(), SDL_BLENDMODE_BLEND))
    throw std::runtime_error(std::string("Cannot enable PNG alpha blending: ") +
                             SDL_GetError());
  texture_ = std::move(next);
  width_ = w;
  height_ = h;
  tile_width_ = tile_width;
  tile_height_ = tile_height;
  path_ = path;
}
TextureRegion Tileset::region(int id) const {
  if (!loaded() || id < 1 || id > tile_count())
    return {0, 0, 0, 0};
  const int index = id - 1, cols = width_ / tile_width_;
  const float u0 = static_cast<float>((index % cols) * tile_width_) / width_;
  const float v0 = static_cast<float>((index / cols) * tile_height_) / height_;
  return {u0, v0, u0 + static_cast<float>(tile_width_) / width_,
          v0 + static_cast<float>(tile_height_) / height_};
}
} // namespace pf
