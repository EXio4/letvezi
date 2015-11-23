/* main.cpp*/


#include <cstdint>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <algorithm>
#include "game.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
namespace Letvetzi {
    struct Velocity {
        public:
        int16_t x;
        int16_t y;
    };
    Velocity vel_new(int16_t x_p, int16_t y_p) {
        Velocity v;
        v.x = x_p;
        v.y = y_p;
        return v;
    };
    struct Position {
        public:
        uint16_t x;
        uint16_t y;
        Velocity vel;

    };

    Position pos_new(uint16_t x_p, uint16_t y_p, Velocity vel_p) {
        Position p;
        p.x   = x_p;
        p.y   = y_p;
        p.vel = vel_p;
        return p;
    }
    struct EntityName {
        enum EntityType {
            EntityPlayer,
            EntityEnemy ,
            EntityBullet,
        } tag;
    };


    namespace Events {
        struct Type {
            enum Tag {
                QuitGame,
                Move,
            } tag;
            union {
                Velocity vel;
            } data;
        };

        Type quitGame() {
            Type e;
            e.tag = Type::QuitGame;
            return e;
        }
        Type move(int16_t x, int16_t y) {
            Type e;
            e.tag = Type::Move;
            e.data.vel.x = x;
            e.data.vel.y = y;
            return e;
        }
    }

    namespace GameState {
        struct Resolution {
            uint16_t height;
            uint16_t width;
        };
        Resolution res_new(uint16_t height_p, uint16_t width_p) {
            Resolution r;
            r.height = height_p;
            r.width  = width_p;
            return r;
        }
        struct Type {
            public:
            Resolution res;
            bool quit;
            Position player;
//            std::map<Entity,Position> ent_mp;
        };
        Type typ_new(Resolution res_p, Position player_p, bool quit_p = false) {
            Type t;
            t.res    = res_p;
            t.quit   = quit_p;
            t.player = player_p;
            return t;
        }
    }
    
    void event_handler(/*Game::sdl_info&           gs         ,*/
                       Conc::Chan<SDL_Event>&    sdl_events ,
                       Conc::Chan<Events::Type>& game_events) {
        while(true) {
            SDL_Event ev = sdl_events.pop();
            switch(ev.type) {
                case SDL_QUIT:
                         game_events.push(Events::quitGame());
                         break;
                case SDL_KEYDOWN:
                        if(ev.key.repeat == 0) {
                            switch(ev.key.keysym.sym) {
                                case SDLK_UP:    game_events.push(Events::move(0,-50));
                                                break;
                                case SDLK_DOWN:  game_events.push(Events::move(0,+50));
                                                break;
                                case SDLK_LEFT:  game_events.push(Events::move(-50,0));
                                                break;
                                case SDLK_RIGHT: game_events.push(Events::move(+50,0));
                                                break;
                                default: break;
                            }
                        }
                        break;
                case SDL_KEYUP:
                        switch(ev.key.keysym.sym) {
                            case SDLK_UP:    game_events.push(Events::move(0,+50));
                                             break;
                            case SDLK_DOWN:  game_events.push(Events::move(0,-50));
                                             break;
                            case SDLK_LEFT:  game_events.push(Events::move(+50,0));
                                             break;
                            case SDLK_RIGHT: game_events.push(Events::move(-50,0));
                                             break;
                            default: break;
                        }
                        break;
                default: break;
            }
        }
    };
    void game_handler(/*Game::sdl_info&               gs          ,*/
                      Conc::Chan<Events::Type>&     game_events ,
                      Conc::VarL<GameState::Type>&  svar        ) {
        while(true) {
            Events::Type ev = game_events.pop();
            switch(ev.tag) {
                case Events::Type::QuitGame:
                    svar.modify([](GameState::Type& s) {
                        s.quit = true;
                    });
                break;
                case Events::Type::Move:
                    svar.modify([&](GameState::Type& s) {
                        s.player.vel.x += ev.data.vel.x;
                        s.player.vel.y += ev.data.vel.y;
                    });
                break;
                default:
                break;
            };
        };
    };

    /* fps_relation, 1000 = 1s
     * 
     * the speed unit used internally, is defined as `1u = 1% of the screen` approx.
     */
    Game::LSit render_handler(Game::sdl_info&              gs,
                              Conc::VarL<GameState::Type>& svar,
                              uint16_t                     fps_relation) {
        return svar.modify([&](GameState::Type& s) {
            { // apply "velocity" to the player
                int16_t p_x = s.player.x + (s.player.vel.x * s.res.height * fps_relation) /100/1000;
                int16_t p_y = s.player.y + (s.player.vel.y * s.res.width  * fps_relation) /100/1000;
                s.player.x = std::max<int16_t>(0, std::min<int16_t>(p_x, s.res.height));
                s.player.y = std::max<int16_t>(0, std::min<int16_t>(p_y, s.res.width));
            }
            // apply background
            SDL_SetRenderDrawColor(gs.win_renderer, 0, 0, 0, 255);
            SDL_RenderClear(gs.win_renderer);

            // draw player
            gs.with("player", [&](SDL_Texture *player) {
                SDL_Rect r;
                r.x = s.player.x;
                r.y = s.player.y;
                r.h = 128;
                r.w = 128;
                SDL_RenderCopy(gs.win_renderer, player, NULL, &r);
            });
            if (s.quit) return Game::BreakLoop;
            return Game::KeepLooping;
        });
    }
};
int main(/*int argc, char** args*/) {

    try {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) { throw Game::SDLError(); }
        { int imgFlags = IMG_INIT_PNG;
          if(!(IMG_Init(imgFlags) & imgFlags)) { throw Game::SDLError(IMG_GetError()); }
        }
        Game::sdl_info gs("Letvetzi", SCREEN_WIDTH, SCREEN_HEIGHT);

        gs.load_png("player", "doge.png");

        Letvetzi::GameState::Type start_state =
                    Letvetzi::GameState::typ_new(
                        Letvetzi::GameState::res_new(640,480),
                        Letvetzi::pos_new(640/2,480/2,
                                Letvetzi::vel_new(0,0))
                        );

        std::function<void(Conc::Chan<SDL_Event>&,Conc::Chan<Letvetzi::Events::Type>&)> event_fn =
                Letvetzi::event_handler; /*
                [&](Conc::Chan<SDL_Event>& s_ev, Conc::Chan<Letvetzi::Events::Type>& g_ev) {
                            return Letvetzi::event_handler(gs, s_ev, g_ev);
                        }; */
        std::function<void(Conc::Chan<Letvetzi::Events::Type>&,Conc::VarL<Letvetzi::GameState::Type>&)> game_fn =
                Letvetzi::game_handler; /*
                [&](Conc::Chan<Letvetzi::Events::Type>& g_ev, Conc::VarL<Letvetzi::GameState::Type>& typ) {
                            return Letvetzi::game_handler(gs, g_ev, typ);
                        };*/
        std::function<Game::LSit(Conc::VarL<Letvetzi::GameState::Type>&,uint16_t)> render_fn =
                [&](Conc::VarL<Letvetzi::GameState::Type>& typ,uint16_t fps_rel) {
                            return Letvetzi::render_handler(gs,typ,fps_rel);
                        };

        gs.loop(event_fn, game_fn, render_fn, start_state);

        printf("Game over!\n");


    } catch (Game::SDLError& err) {
        printf("SDL Error: %s\n", err.what());
    }
    SDL_Quit();

    return 0;
}