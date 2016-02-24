/* main.cpp*/


#include <cstdint>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <list>
#include <random>
#include <algorithm>
#include "game.h"
#include "letvezi.h"
#include "render.h"

#ifdef _WIN32
int WinMain() {
#else
int main() {
#endif

    try {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) { throw Game::SDLError(); }
        { int imgFlags = IMG_INIT_PNG;
          if(!(IMG_Init(imgFlags) & imgFlags)) { throw Game::SDLError(IMG_GetError()); }
        }
        if (TTF_Init() < 0) { throw Game::SDLError(TTF_GetError()); }
        if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            printf("Error starting SDL_Mix: %s\n", Mix_GetError());
        }
        auto gs = std::make_shared<Game::sdl_info>("Letvezi", "art/font.ttf");
        auto persistent = std::make_shared<Persistent>("user_info.dat");

        if (persistent == NULL) throw std::runtime_error("Failed to create user settings object");

        gs->loading_screen([persistent](auto& ch) {
            auto set_bg   = [&ch](std::string file) {
                                ch.push(std::tuple<std::string,std::function<void(Game::sdl_info&)>>(
                                        "background music",
                                        [file](auto& gs) { gs.set_background(file); }
                                    )
                                );
                            };
            auto load_sfx = [&ch](std::string key, Game::SFX_ID xkey, std::string file) {
                                ch.push( std::tuple<std::string,std::function<void(Game::sdl_info&)>>(
                                        key,
                                        [xkey,file](auto& gs) { gs.load_sfx(xkey,file); }
                                    )
                                );
                            };
            auto load_png = [&ch](std::string key, Game::TextureID xkey, std::string file) {
                                ch.push( std::tuple<std::string,std::function<void(Game::sdl_info&)>>(
                                        key,
                                        [xkey,file](auto& gs) { gs.load_png(xkey,file); }
                                    )
                                );
                            };
            auto do_ = [&ch](std::string key, std::function<void()> fn) {
                                ch.push( std::tuple<std::string,std::function<void(Game::sdl_info&)>>(
                                        key,
                                        [fn](auto&) {
                                            fn();
                                        }
                                ));
                        };


            set_bg("art/background.ogg");

            load_png("bg_star"         , Game::TEX_BackgroundStar   , "art/img/bg_star.png"            );

            load_png("player"          , Game::TEX_Player           , "art/img/player.png"             );
            load_png("player_laser"    , Game::TEX_PlayerLaser      , "art/img/player_laser.png"       );
            load_png("player_life"     , Game::TEX_PlayerLife       , "art/img/player_life.png"        );
            load_png("player_shield"   , Game::TEX_PlayerShield     , "art/img/player_shield.png"      );

            load_png("enemy_1"         , Game::TEX_Enemy1           , "art/img/enemy_1.png"            );
            load_png("enemy_2"         , Game::TEX_Enemy2           , "art/img/enemy_2.png"            );
            load_png("enemy_3"         , Game::TEX_Enemy3           , "art/img/enemy_3.png"            );
            load_png("enemy_boss"      , Game::TEX_EnemyBoss        , "art/img/enemy_boss.png"         );
            load_png("enemy_laser"     , Game::TEX_EnemyLaser       , "art/img/enemy_laser.png"        );
            load_png("enemy_boss_laser", Game::TEX_EnemyBossLaser   , "art/img/enemy_boss_laser.png"   );
            load_png("enemy_boss_squad", Game::TEX_EnemyBossSquad   , "art/img/enemy_boss_squad.png"   );

            load_png("powerup_shield"  , Game::TEX_PowerupShield    , "art/img/powerup_shield.png"     );
            load_png("powerup_bolt"    , Game::TEX_PowerupBolt      , "art/img/powerup_bolt.png"       );

            load_sfx("player_laser"    , Game::SFX_PlayerLaser      , "art/sfx/player_laser.ogg"       );
            load_sfx("shield_enabled"  , Game::SFX_ShieldEnabled    , "art/sfx/player_laser.ogg"       );
            do_("user info", [persistent]() { persistent->load(); });

        });

        Game::Resolution res = gs->get_current_res();
        Letvezi::GameState::Type start_state =
                   Letvezi::GameState::Type(persistent, gs, res,
                            Letvezi::Position(res.width/2, res.height-70-gs->textures().at(Game::TEX_Player).height));

        std::function<void(Conc::Chan<SDL_Event>&,Conc::Chan<Letvezi::Events::Type>&)> event_fn =
                Letvezi::event_handler;
        std::function<void(Conc::Chan<Letvezi::Events::Type>&,Conc::VarL<Letvezi::GameState::Type>&)> game_fn =
                Letvezi::game_handler;
        std::function<Game::LSit(Conc::VarL<Letvezi::GameState::Type>&,uint16_t)> render_fn =
                [gs](Conc::VarL<Letvezi::GameState::Type>& typ,uint16_t fps_rel) {
                            return Letvezi::Render::handler_game(gs,typ,fps_rel);
                        };
        std::function<void(Conc::VarL<Letvezi::GameState::Type>&)> expensive_handler =
                Letvezi::Render::expensive_handler;


        gs->loop(event_fn, game_fn, expensive_handler, render_fn, start_state);
        persistent->save();
        printf("Game over!\n");
    } catch (Game::SDLError& err) {
        printf("SDL Error: %s\n", err.what());
    }
    std::cout << "SDL_Quit()" << std::endl;
    SDL_Quit();

    return 0;
}
