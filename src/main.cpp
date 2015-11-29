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
        if (TTF_Init() < 0) { throw Game::SDLError(TTF_GetError()); }
        if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            throw Game::SDLError(Mix_GetError());
        }
        Game::sdl_info gs("Letvetzi", "art/font.ttf");

        gs.load_png("bg_star"      , "art/img/bg_star.png");

        gs.load_png("player"       , "art/img/player.png");
        gs.load_png("player_laser" , "art/img/player_laser.png");
        gs.load_png("player_life"  , "art/img/player_life.png");
        gs.load_png("player_shield", "art/img/player_shield.png");

        gs.load_png("enemy_1"      , "art/img/enemy_1.png");
        gs.load_png("enemy_2"      , "art/img/enemy_2.png");
        gs.load_png("enemy_3"      , "art/img/enemy_3.png");
        gs.load_png("enemy_boss"   , "art/img/enemy_boss.png");
        gs.load_png("enemy_laser"  , "art/img/enemy_laser.png");

        gs.load_png("powerup_shield", "art/img/powerup_shield.png");
        gs.load_png("powerup_bolt"  , "art/img/powerup_bolt.png");

        gs.load_sfx("player_laser" , "art/sfx/player_laser.ogg");
        gs.load_sfx("shield_enabled", "art/sfx/player_laser.ogg");

        Game::Resolution res = gs.get_current_res();
        Letvetzi::GameState::Type start_state =
                   Letvetzi::GameState::Type(res,
                            Letvetzi::Position(res.width/2, res.height-70-gs.textures().at("player").height),
                                &gs);

        std::function<void(Conc::Chan<SDL_Event>&,Conc::Chan<Letvetzi::Events::Type>&)> event_fn =
                Letvetzi::event_handler;
        std::function<void(Conc::Chan<Letvetzi::Events::Type>&,Conc::VarL<Letvetzi::GameState::Type>&)> game_fn =
                Letvetzi::game_handler;
        std::function<Game::LSit(Conc::VarL<Letvetzi::GameState::Type>&,uint16_t)> render_fn =
                [&](Conc::VarL<Letvetzi::GameState::Type>& typ,uint16_t fps_rel) {
                            return Letvetzi::Render::handler_game(gs,typ,fps_rel);
                        };

        gs.loop(event_fn, game_fn, render_fn, start_state);

        printf("Game over!\n");


    } catch (Game::SDLError& err) {
        printf("SDL Error: %s\n", err.what());
    }
    SDL_Quit();

    return 0;
}