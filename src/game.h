#pragma once

#include <SDL2/SDL.h>
#include <map>
#include <stdexcept>

namespace game {

    class SDLError: public std::runtime_error {
        private:
            std::string info;
        public:

        SDLError(std::string x)
            : std::runtime_error("SDL Error")
            { info = x; }
        SDLError(const char* x)
            : std::runtime_error("SDL Error")
            { info = x; }
        SDLError()
            : std::runtime_error("SDL Error")
            { info = SDL_GetError(); }

        virtual const char* what() const throw() {
            /*std::ostringstream cnvt;

            cnvt.str("SDL Error: ");

            cnvt << info; */

            return info.c_str();
        }
    };

    class sdl_info {
        public:
            SDL_Window*  window         = NULL;
            SDL_Surface* window_surface = NULL;
            std::map<std::string, SDL_Surface*> map;
            /* std::string til I manage to stop being lazy */

            sdl_info(int width, int height) {
                window = SDL_CreateWindow("Letvezi", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
                if (window == NULL) {
                    throw SDLError();
                }
                window_surface = SDL_GetWindowSurface(window);
            }
            ~sdl_info() {
                SDL_DestroyWindow(window);
                for (const auto &pair : map) {
                    SDL_FreeSurface(pair.second);
                }
            }
    };
}