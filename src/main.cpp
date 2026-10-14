#include "application.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <exception>
#include <iostream>
#include <filesystem>
#include <string_view>
int main(int argc, char **argv) {
  try {
    if (argc > 1 && std::string_view(argv[1]) == "--visual-test") {
      if (argc != 4 || std::string_view(argv[2]) != "--output-dir") {
        std::cerr << "Usage: PixelForge --visual-test --output-dir <directory>\n";
        return 2;
      }
      return pf::run(false, std::filesystem::path(argv[3]));
    }
    return pf::run(argc > 1 && std::string_view(argv[1]) == "--smoke");
  } catch (const std::exception &e) {
    std::cerr << "PixelForge: " << e.what() << '\n';
    return 1;
  }
}
