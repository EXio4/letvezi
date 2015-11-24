/* main.cpp*/


#include <cstdint>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <list>
#include <random>
#include <algorithm>
#include "game.h"

const int SCREEN_WIDTH  = 640;
const int SCREEN_HEIGHT = 480;

namespace Letvetzi {
    struct Resolution {
        uint16_t width;
        uint16_t height;
        Resolution(uint16_t width, uint16_t height) : width(width), height(height) {
        }
    };
    struct Velocity {
        public:
        int16_t x;
        int16_t y;
        Velocity(int16_t x_p, int16_t y_p) : x(x_p), y(y_p) {
        };
    };
    struct Position {
        public:
        uint16_t x;
        uint16_t y;
        Velocity vel;
        Position(uint16_t x_p, uint16_t y_p, Velocity vel_p) : x(x_p), y(y_p), vel(vel_p) {
        };
        void apply_vel(Resolution res, int16_t fps_relation) {
            int16_t p_x = x + (vel.x * res.width * fps_relation) /100/1000;
            int16_t p_y = y + (vel.y * res.height  * fps_relation) /100/1000;
            x = std::max<int16_t>(0, std::min<int16_t>(p_x, res.width));
            y = std::max<int16_t>(0, std::min<int16_t>(p_y, res.height));
        }
    };

    struct Particle {
        std::string txt_name;
        Position pos;
        Particle(std::string txt_name_p, Position pos_p) : txt_name(txt_name_p), pos(pos_p) {
        };
    };

    struct EntityName {
        uint16_t entity_id;
    };


    namespace Events {
        struct Type {
            enum Tag {
                QuitGame,
                Move,
            } tag;
            struct {
                Velocity vel = Velocity(0,0);
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
            e.data.vel = Velocity(x,y);
            return e;
        }
    }

    namespace GameState {

        struct Type {
            public:
            Resolution res;
            bool quit;
            Position player;
            std::list<Particle> bg_particles;
            struct {
                std::default_random_engine random_eng;
                std::uniform_int_distribution<uint16_t> start_pos;
                std::uniform_int_distribution<int16_t> start_speed;
            } bg_particles_gen;
            std::map<EntityName,Position> ent_mp;
            Type(Resolution res_p, Position player_p, bool quit_p = false)
               : res(res_p), quit(quit_p), player(player_p) {
                { std::random_device rd;
                  std::default_random_engine r_eg(rd());;
                  std::uniform_int_distribution<uint16_t> start_pos(0, res.width); // start positions \x -> (x,0)
                  std::uniform_int_distribution<int16_t> start_speed(40,70); // speed
                  bg_particles_gen.random_eng = r_eg;
                  bg_particles_gen.start_pos = start_pos;
                  bg_particles_gen.start_speed = start_speed;
                };
                {
                    for (int i=0; i<80; i++) {
                        add_bg_particle();
                    }
                };

            };
            void add_bg_particle() {
                uint16_t x     = bg_particles_gen.start_pos  (bg_particles_gen.random_eng);
                int16_t  speed = bg_particles_gen.start_speed(bg_particles_gen.random_eng);
                bg_particles.push_front(Particle("bg_star", Position(x,0,Velocity(0, speed))));
            };
        };
    }

    void event_handler(Conc::Chan<SDL_Event>&    sdl_events ,
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
    void game_handler(Conc::Chan<Events::Type>&     game_events ,
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
    namespace Render {
        Game::LSit handler(Game::sdl_info&              gs,
                                Conc::VarL<GameState::Type>& svar,
                                uint16_t                     fps_relation) {
            return svar.modify([&](GameState::Type& s) {
                s.player.apply_vel(s.res, fps_relation);
                // apply background
                SDL_SetRenderDrawColor(gs.win_renderer, 0, 0, 0, 255);
                SDL_RenderClear(gs.win_renderer);
                for(auto p = s.bg_particles.begin(); p != s.bg_particles.end() ;) {
                    (*p).pos.apply_vel(s.res, fps_relation);
                    gs.with("bg_star", [&](SDL_Texture *bg_star) {
                        SDL_Rect r;
                        r.x = (*p).pos.x;
                        r.y = (*p).pos.y;
                        SDL_QueryTexture(bg_star, NULL, NULL, &(r.w), &(r.h));
                        SDL_RenderCopy(gs.win_renderer, bg_star, NULL, &r);
                    });
                    if ((*p).pos.y >= s.res.height) {
                        p = s.bg_particles.erase(p);
                        s.add_bg_particle();
                    } else {
                        ++p;
                    }
                }
                /**/

                // draw player
                gs.with("player", [&](SDL_Texture *player) {
                    SDL_Rect r;
                    r.x = s.player.x;
                    r.y = s.player.y;
                    SDL_QueryTexture(player, NULL, NULL, &(r.w), &(r.h));
                    SDL_RenderCopy(gs.win_renderer, player, NULL, &r);
                });
                if (s.quit) return Game::BreakLoop;
                return Game::KeepLooping;
            });
        };
    }
};
int main(/*int argc, char** args*/) {

    try {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) { throw Game::SDLError(); }
        { int imgFlags = IMG_INIT_PNG;
          if(!(IMG_Init(imgFlags) & imgFlags)) { throw Game::SDLError(IMG_GetError()); }
        }
        Game::sdl_info gs("Letvetzi", SCREEN_WIDTH, SCREEN_HEIGHT);

        gs.load_png("player" , "player.png");
        gs.load_png("bg_star", "bg_star.png");
        gs.load_png("enemy_1", "enemy_1.png");

        Letvetzi::GameState::Type start_state =
                   Letvetzi::GameState::Type(
                        Letvetzi::Resolution(SCREEN_WIDTH,SCREEN_HEIGHT),
                        Letvetzi::Position(SCREEN_WIDTH/2,SCREEN_HEIGHT-SCREEN_HEIGHT/4,
                                Letvetzi::Velocity(0,0))
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