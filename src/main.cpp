/* main.cpp*/

#include <SDL2/SDL.h>
#include <stdio.h>
#include "game.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char** args) {

    try {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) { throw game::SDLError(); }

        game::sdl_info gs(SCREEN_WIDTH, SCREEN_HEIGHT);

        for (int i=0; i<255; i++) {
            SDL_FillRect(gs.window_surface, NULL, SDL_MapRGB(gs.window_surface->format, 0xAC, i, 0xAC ) );
            SDL_UpdateWindowSurface(gs.window);
            SDL_Delay(25);
        }

    } catch (game::SDLError& err) {
        printf("SDL Error: %s\n", err.what());
    }
    SDL_Quit();

    return 0;
}