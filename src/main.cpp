/* main.cpp*/


#include <cstdint>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <list>
#include <random>
#include <algorithm>
#include "game.h"
#include "letvetzi.h"

int main() {

    try {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) { throw Game::SDLError(); }
        { int imgFlags = IMG_INIT_PNG;
          if(!(IMG_Init(imgFlags) & imgFlags)) { throw Game::SDLError(IMG_GetError()); }
        }
        Game::sdl_info gs("Letvetzi");

        gs.load_png("player"      , "art/player.png");
        gs.load_png("bg_star"     , "art/bg_star.png");
        gs.load_png("player_laser", "art/player_laser.png");
        gs.load_png("enemy_1"     , "art/enemy_1.png");

        Game::Resolution res = gs.get_current_res();
        Letvetzi::GameState::Type start_state =
                   Letvetzi::GameState::Type(res,
                            Letvetzi::Position(res.width/2,res.height-64-gs.textures().at("player").height,
                            Letvetzi::Velocity(0,0))
                        );

        std::function<void(Conc::Chan<SDL_Event>&,Conc::Chan<Letvetzi::Events::Type>&)> event_fn =
                Letvetzi::event_handler;
        std::function<void(Conc::Chan<Letvetzi::Events::Type>&,Conc::VarL<Letvetzi::GameState::Type>&)> game_fn =
                Letvetzi::game_handler;
        std::function<Game::LSit(Conc::VarL<Letvetzi::GameState::Type>&,uint16_t)> render_fn =
                [&](Conc::VarL<Letvetzi::GameState::Type>& typ,uint16_t fps_rel) {
                            return Letvetzi::Render::handler(gs,typ,fps_rel);
                        };

        gs.loop(event_fn, game_fn, render_fn, start_state);

        printf("Game over!\n");


    } catch (Game::SDLError& err) {
        printf("SDL Error: %s\n", err.what());
    }
    SDL_Quit();

    return 0;
}