#include "application.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <string_view>
#include <exception>
int main(int argc,char** argv) {
 try{return pf::run(argc>1 && std::string_view(argv[1])=="--smoke");}
 catch(const std::exception& e){std::cerr<<"PixelForge: "<<e.what()<<'\n';return 1;}
}
