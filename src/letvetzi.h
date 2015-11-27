#include <cstdint>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <list>
#include <random>
#include <algorithm>
#include <boost/variant.hpp>
#include "game.h"

namespace Letvetzi {
    struct Velocity {
        public:
        int16_t x;
        int16_t y;
        Velocity(int16_t x_p, int16_t y_p) : x(x_p), y(y_p) {
        };
    };
    struct Position {
        public:
        int16_t x;
        int16_t y;
        Velocity vel;
        Position(int16_t x_p, int16_t y_p, Velocity vel_p) : x(x_p), y(y_p), vel(vel_p) {
        };
        Position() : x(0), y(0), vel(Velocity(0,0)) {};
        void apply_vel(Game::Resolution res, int16_t fps_relation) {
            int16_t p_x = x + (vel.x * res.width * fps_relation) /100/1000;
            int16_t p_y = y + (vel.y * res.height  * fps_relation) /100/1000;
            x = std::max<int16_t>(-64, std::min<int16_t>(p_x, res.width+64));
            y = std::max<int16_t>(-64, std::min<int16_t>(p_y, res.height+64));
        }
    };

    struct Particle {
        std::string txt_name;
        Position pos;
        Particle(std::string txt_name_p, Position pos_p) : txt_name(txt_name_p), pos(pos_p) {
        };
    };

    namespace GameState {
        class Type;
    };

    namespace Entity {
        struct Meta;
        struct Name {
            uint16_t entity_id;
            Name(uint16_t e) : entity_id(e) {};
            Name& operator++() {
                entity_id++;
                return *this;
            };
            bool operator==(const Name& other) const {
                return entity_id == other.entity_id;
            };
            bool operator!=(const Name& other) const {
                return entity_id != other.entity_id;
            };
            bool operator>=(const Name& other) const {
                return entity_id >= other.entity_id;
            };
            bool operator<=(const Name& other) const {
                return entity_id <= other.entity_id;
            };
            bool operator< (const Name& other) const {
                return entity_id <  other.entity_id;
            };
            bool operator> (const Name& other) const {
                return entity_id >  other.entity_id;
            };
        };

        class Bullet {
            public:
                Bullet() {
                };
                void out_of_screen(GameState::Type&, Meta&, Position&);

        };
        class Enemy {
            public:
                Enemy() {
                };
                void out_of_screen(GameState::Type&, Meta&, Position&);
        };

        class OutOfScreen {
            Meta&     ent;
            Position& pos;
            GameState:Type& gs;
            public:
                OutOfScreen(GameState::Type& gs, Meta& ent, Position& pos) : ent(ent), pos(pos), gs(gs) {}
                void operator()(Bullet x) {
                    x.out_of_screen(gs, ent, pos);
                }
                void operator()(Enemy x) {
                    x.out_of_screen(gs, ent, pos);
                }
        };

        struct Meta {
            std::string txt_name;
            bool killed = false;
            Position pos;
            SDL_Rect rect;
            boost::variant<Bullet, Enemy> var;
            OutOfScreen out_of_screen/* = OutOfScreen(gs, *this, pos)*/;
            Meta(GameState::Type& gs) : out_of_screen(OutOfScreen(gs, *this, pos)) { };
            void kill() {
                killed = true;
            };
        };
    };


    namespace GameState {

        class Type {
            public:
            Game::Resolution res;
            bool quit;
            Position player;
            std::list<Particle> bg_particles;
            struct {
                std::default_random_engine random_eng;
                std::uniform_int_distribution<int16_t> start_pos;
                std::uniform_int_distribution<int16_t> start_speed;
            } bg_particles_gen;
            std::map<Entity::Name,Entity::Meta> ent_mp;
            Entity::Name last_entity = Entity::Name(0);
            Type(Game::Resolution res_p, Position player_p, bool quit_p = false)
               : res(res_p), quit(quit_p), player(player_p) {
                { std::random_device rd;
                  std::default_random_engine r_eg(rd());;
                  std::uniform_int_distribution<int16_t> start_pos(0, res.width); // start positions \x -> (x,0)
                  std::uniform_int_distribution<int16_t> start_speed(40,100); // speed
                  bg_particles_gen.random_eng = r_eg;
                  bg_particles_gen.start_pos = start_pos;
                  bg_particles_gen.start_speed = start_speed;
                };
                {
                    for (int i=0; i<res.width/20; i++) {
                        add_bg_particle((i*100)%res.height);
                    };

                    for (int i=0; i<5; i++) {
                        add_enemy();
                    };
                };

            };
            void add_bg_particle(int16_t start_y=0) {
                int16_t x     = bg_particles_gen.start_pos  (bg_particles_gen.random_eng);
                int16_t speed = bg_particles_gen.start_speed(bg_particles_gen.random_eng);
                int16_t ysped = bg_particles_gen.start_speed(bg_particles_gen.random_eng);
                bg_particles.push_front(Particle("bg_star", Position(x,start_y,Velocity(ysped-70, speed))));
            };

            void add_enemy() {
                with_new_entity([&](Entity::Name, Entity::Meta& entity) {
                    int16_t x     = bg_particles_gen.start_pos  (bg_particles_gen.random_eng);
                    // we reuse the random number generator of the bg_particles, TODO: rename it
                    entity.pos = Position(x, 15, Velocity(0,50));
                    entity.txt_name = "enemy_1";
                });
            };

            void add_bullet(Velocity vel) {
                with_new_entity([&](Entity::Name, Entity::Meta& entity) {
                    entity.pos      = player;
                    entity.pos.x   += 32;
                    entity.txt_name = "player_laser";
                    entity.pos.vel  = vel;
                });
            }

            void with_new_entity(std::function<void(Entity::Name,Entity::Meta&)> fn) {
                 Entity::Name e = last_entity;
                 ++last_entity;
                 return fn(e,ent_mp[e]);
            }
        };
    }

    namespace Entity {
        void on_collision(GameState::Type& gs, Meta& ent_x, Meta& ent_y) {
                        if ((ent_x.var.type() == typeid(Bullet) && ent_y.var.type() == typeid(Enemy) ) ||
                            (ent_x.var.type() == typeid(Enemy)  && ent_y.var.type() == typeid(Bullet)) ) {
                                ent_x.kill();
                                ent_y.kill();
                                gs.add_enemy();
//                        gs.add_points(100);
                        };
        };
        void Bullet::out_of_screen(GameState::Type&, Meta& ent, Position&) {
            ent.kill();
        };
        void Enemy::out_of_screen(GameState::Type& gs, Meta& ent, Position&) {
            ent.kill();
            gs.add_enemy();
        };
    }

    namespace Events {
        class QuitGame {
            public:
                QuitGame() { };
        };
        class PlayerMove {
            public:
                Velocity vel;
                PlayerMove(int16_t x, int16_t y) : vel(Velocity(x,y)){
                }
        };

        class Shoot {
            public:
                Velocity vel;

                Shoot(int16_t accel)  : vel(Velocity(0,-accel)) {
                };
        };

        class ApplyEvent : public boost::static_visitor<void> {
        private:
            GameState::Type& s;
        public:
            ApplyEvent(GameState::Type& s) : s(s) {};
            void operator()(Shoot ev) const {
                s.add_bullet(ev.vel);
            };
            void operator()(PlayerMove ev) const {
                s.player.vel.x += ev.vel.x;
                s.player.vel.y += ev.vel.y;
            };
            void operator()(QuitGame) const {
                s.quit = true;
            }
        };

        struct Type {
            public:
                boost::variant<QuitGame, PlayerMove, Shoot> data;
            Type(boost::variant<QuitGame, PlayerMove, Shoot> data) : data(data) {};
        };
    }


    void event_handler(Conc::Chan<SDL_Event>&    sdl_events ,
                       Conc::Chan<Events::Type>& game_events) {
        while(true) {
            SDL_Event ev = sdl_events.pop();
            switch(ev.type) {
                case SDL_QUIT:
                         game_events.push(Events::Type(Events::QuitGame()));
                         break;
                case SDL_KEYDOWN:
                        if(ev.key.repeat == 0) {
                            switch(ev.key.keysym.sym) {
                                case SDLK_LEFT:  game_events.push(Events::Type(Events::PlayerMove(-50,0)));
                                                  break;
                                case SDLK_RIGHT: game_events.push(Events::Type(Events::PlayerMove(+50,0)));

                                                 break;
                            }
                        } 
                        switch(ev.key.keysym.sym) {
                            case SDLK_SPACE: game_events.push(Events::Type(Events::Shoot(150)));
                                             break;
                        }
                        break;
                case SDL_KEYUP:
                        switch(ev.key.keysym.sym) {
                            case SDLK_LEFT:  game_events.push(Events::Type(Events::PlayerMove(+50,0)));
                                             break;
                            case SDLK_RIGHT: game_events.push(Events::Type(Events::PlayerMove(-50,0)));
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
            svar.modify([&](GameState::Type& s) {
                boost::apply_visitor(Events::ApplyEvent(s), ev.data);
            });
        };
    };

    /* fps_relation, 1000 = 1s
     * 
     * the speed unit used internally, is defined as `1u = 1% of the screen` approx.
     */
    namespace Render {
        bool collide(const SDL_Rect *r1, const SDL_Rect *r2) {
            return (SDL_HasIntersection(r1,r2) == SDL_TRUE);
        };

        Game::LSit handler(Game::sdl_info&              gs,
                                Conc::VarL<GameState::Type>& svar,
                                uint16_t                     fps_relation) {
            return svar.modify([&](GameState::Type& s) {
                s.player.apply_vel(s.res, fps_relation);
                // apply background
                SDL_SetRenderDrawColor(gs.win_renderer, 75, 0, 60, 255);
                SDL_RenderClear(gs.win_renderer);
                for(auto p = s.bg_particles.begin(); p != s.bg_particles.end() ;) {
                    gs.with("bg_star", [&](Game::TextureInfo bg_star) {
                        p->pos.apply_vel(s.res, fps_relation);
                        /* this could be optimized such that we don't query the texture more than once
                         *  (per frame, or even `just once` in the whole game)
                         */
                        SDL_Rect r;
                        r.x = p->pos.x;
                        r.y = p->pos.y;
                        r.w = bg_star.width;
                        r.h = bg_star.height;
                        SDL_RenderCopy(gs.win_renderer, bg_star.texture, NULL, &r);
                    });

                    if (p->pos.y >= s.res.height || p->pos.x <= 0 || p->pos.x >= s.res.width) {
                        p = s.bg_particles.erase(p);
                        s.add_bg_particle();
                    } else {
                        p++;
                    }
                }
                /**/

                /* draw entities on the world */
                for (auto curr = s.ent_mp.begin() ; curr != s.ent_mp.end() ;) {
                    gs.with(curr->second.txt_name, [&](Game::TextureInfo text) {
                        curr->second.pos.apply_vel(s.res, fps_relation);
                        SDL_Rect curr_rect;
                        curr_rect.x = curr->second.pos.x;
                        curr_rect.y = curr->second.pos.y;
                        curr_rect.w = text.width;
                        curr_rect.h = text.height;
                        SDL_RenderCopy(gs.win_renderer, text.texture, NULL, &curr_rect);

                        if ((curr->second.pos.x > s.res.width  || curr->second.pos.x < 0) ||
                            (curr->second.pos.y > s.res.height || curr->second.pos.y < 0) ) {
                                boost::apply_visitor(curr->second.out_of_screen, curr->second.var);
                        };

                        // collision detection
                        for (auto& other : s.ent_mp) {
                            if (other.first == curr->first) continue; // we ignore ourselves
                            gs.with(other.second.txt_name, [&](Game::TextureInfo text2) {
                                SDL_Rect other_rect;
                                other_rect.x = other.second.pos.x;
                                other_rect.y = other.second.pos.y;
                                other_rect.w = text2.width;
                                other_rect.h = text2.height;
                                if (collide(&curr_rect, &other_rect)) {
                                    Entity::on_collision(s, curr->second, other.second);
                                };
                            });
                        };
                    });
                    if (curr->second.killed) {
                        curr = s.ent_mp.erase(curr);
                    } else {
                        curr++;
                    };
                }

                // draw player
                gs.with("player", [&](Game::TextureInfo player) {
                    SDL_Rect r;
                    r.x = s.player.x;
                    r.y = s.player.y;
                    r.w = player.width;
                    r.h = player.height;
                    SDL_RenderCopy(gs.win_renderer, player.texture, NULL, &r);
                });

                if (s.quit) return Game::BreakLoop;
                return Game::KeepLooping;
            });
        };
    }
}