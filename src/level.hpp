#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace pf {
enum class Genre { top_down_rpg, platformer };
std::string genre_name(Genre genre);
Genre parse_genre(const std::string& name);
struct Layer { std::string name; std::vector<int> tiles; bool operator==(const Layer&) const = default; };
class Level {
public:
 static constexpr int max_dimension = 256;
 Genre genre;
 int width, height;
 std::vector<Layer> layers;
 Level(Genre genre = Genre::top_down_rpg, int width = 32, int height = 24);
 int tile(int layer, int x, int y) const;
 void paint(int layer, int x, int y, int value);
 void validate() const;
 bool operator==(const Level&) const = default;
private:
 std::size_t index(int layer, int x, int y) const;
};
}
