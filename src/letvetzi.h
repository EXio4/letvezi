#include <cstdint>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <list>
#include <random>
#include <algorithm>
#include <memory>
#include <boost/variant.hpp>
#include "vect.h"
#include "game.h"

namespace Letvetzi {
    struct VelocityT {
        typedef int16_t scalar;
    };
    typedef Vec<VelocityT> Velocity;

    struct PositionT {
        typedef int16_t scalar;
    };
    typedef Vec<PositionT> Position;

    void apply_velocity(Position& pos, Velocity vel, Game::Resolution res, int16_t fps_relation) {
        int16_t p_x = pos.x + (vel.x * res.width  * fps_relation) /100/1000;
        int16_t p_y = pos.y + (vel.y * res.height * fps_relation) /100/1000;
        pos.x = std::max<int16_t>(-64, std::min<int16_t>(p_x, res.width+64));
        pos.y = std::max<int16_t>(-64, std::min<int16_t>(p_y, res.height+64));
    };

    struct Particle {
        std::string txt_name;
        Position pos;
        Velocity vel;
        double angle = 0;
        Particle(std::string txt_name_p, Position pos_p, Velocity vel_p) : txt_name(txt_name_p), pos(pos_p), vel(vel_p) {
        };
    };

    namespace GameState {
        class Type;
    };

    namespace Entity {
        class Type;
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

        class Type {
            public:
                std::string txt_name;
                bool killed = false;
                Position pos = Position(0,0);
                Velocity vel = Velocity(0,0);
                SDL_Rect rect;
                void kill() {
                    killed = true;
                };
                virtual void out_of_screen(GameState::Type&) = 0;
        };

        class Bullet : public Type {
            public:
                Bullet() {};
                void out_of_screen(GameState::Type&);

        };
        class Enemy : public Type {
            public:
                int score = 100;
                Enemy(int score) : score(score) {};
                Enemy() {};
                void out_of_screen(GameState::Type&);
        };

        class Collision {
        public:
            bool operator()(GameState::Type& gs, std::shared_ptr<Type> x_sp, std::shared_ptr<Type> y_sp) {
                Type* x = x_sp.get();
                Type* y = y_sp.get();
                if (auto x_ = dynamic_cast<Enemy*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Bullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                } else if (auto x_ = dynamic_cast<Bullet*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                };

                return false;
            };

            bool on_collision(GameState::Type&, Enemy& , Enemy& );
            bool on_collision(GameState::Type&, Enemy& , Bullet&);
            bool on_collision(GameState::Type&, Bullet&, Enemy& );
            bool on_collision(GameState::Type&, Bullet&, Bullet&);
        } collision;
    };


    namespace GameState {

        class Type {
            public:
            unsigned int points = 0;
            unsigned int lives  = 10;
            bool game_over = false;
            Game::Resolution res;
            bool quit;
            Position player;
            Velocity player_vel;
            Position player_original;
            std::list<Particle> bg_particles;
            struct {
                std::default_random_engine random_eng;
                std::uniform_int_distribution<int16_t> start_pos;
                std::uniform_int_distribution<int16_t> start_speed;
                std::uniform_real_distribution<double> enemy_type;
            } bg_particles_gen;
            std::map<Entity::Name,std::shared_ptr<Entity::Type>> ent_mp;
            Entity::Name last_entity = Entity::Name(0);
            Type(Game::Resolution res_p, Position player_p, bool quit_p = false)
               : res(res_p), quit(quit_p), player(player_p), player_vel(Velocity(0,0)), player_original(player_p) {
                { std::random_device rd;
                  std::default_random_engine r_eg(rd());;
                  std::uniform_int_distribution<int16_t> start_pos(0, res.width); // start positions \x -> (x,0)
                  std::uniform_int_distribution<int16_t> start_speed(40,100); // speed
                  std::uniform_real_distribution<double> enemy_type(0,1); // enemy typ[e
                  bg_particles_gen.random_eng = r_eg;
                  bg_particles_gen.start_pos = start_pos;
                  bg_particles_gen.start_speed = start_speed;
                  bg_particles_gen.enemy_type  = enemy_type;
                };
                {
                    for (int i=0; i<res.width/20; i++) {
                        add_bg_particle((i*100)%res.height);
                    };

                    restart_game();
                };

            };

            void add_bg_particle(int16_t start_y=0) {
                int16_t x     = bg_particles_gen.start_pos  (bg_particles_gen.random_eng);
                int16_t speed = bg_particles_gen.start_speed(bg_particles_gen.random_eng);
                int16_t ysped = bg_particles_gen.start_speed(bg_particles_gen.random_eng);
                bg_particles.push_front(Particle("bg_star", Position(Vec<PositionT>(x,start_y)), Velocity(ysped-70, speed)));
            };

            void add_enemy() {
                with_new_entity([&](Entity::Name) {
                    int16_t x     = bg_particles_gen.start_pos  (bg_particles_gen.random_eng);
                    double  type  = bg_particles_gen.enemy_type (bg_particles_gen.random_eng);
                    // we reuse the random number generator of the bg_particles, TODO: rename it
                    if (type > 0.9) {
                        Entity::Enemy *entity = new Entity::Enemy(300);
                        entity->pos = Position(x, 2);
                        entity->vel = Velocity(0,55);
                        entity->txt_name = "enemy_3";
                        return static_cast<Entity::Type*>(entity);
                    } else if (type > 0.4) {
                        Entity::Enemy *entity = new Entity::Enemy(200);
                        entity->pos = Position(x, 2);
                        entity->vel = Velocity(0,45);
                        entity->txt_name = "enemy_2";
                        return static_cast<Entity::Type*>(entity);
                    } else {
                        Entity::Enemy *entity = new Entity::Enemy(100);
                        entity->pos = Position(x, 5);
                        entity->vel = Velocity(0,35);
                        entity->txt_name = "enemy_1";
                        return static_cast<Entity::Type*>(entity);
                    } 

                });
            };
            void maybe_add_enemy(double prob) {
                double  type  = bg_particles_gen.enemy_type (bg_particles_gen.random_eng);
                if (type > 1-prob) add_enemy();
            };

            void add_bullet(Velocity vel) {
                if (game_over) return restart_game();
                with_new_entity([&](Entity::Name) {
                    Entity::Bullet *entity = new Entity::Bullet();
                    entity->pos        = player;
                    entity->pos.x     += 46;
                    entity->txt_name   = "player_laser";
                    entity->vel        = vel;
                    return entity;
                });
            }
            void restart_game() {
                player = player_original;
                points = 0;
                lives  = 10;
                game_over = false;
                ent_mp.clear();
                last_entity = Entity::Name(0);

                for (int i=0; i<5; i++) {
                    add_enemy();
                };
            };

            void with_new_entity(std::function<Entity::Type*(Entity::Name)> fn) {
                 Entity::Name e = last_entity;
                 ++last_entity;
                 ent_mp[e] = std::shared_ptr<Entity::Type>(fn(e));
            }
            void add_points(unsigned int p) {
                points += p;
            };
        };
    }

    namespace Entity {
        bool Collision::on_collision(GameState::Type& gs, Enemy& e, Bullet& b) {
                    return Collision::on_collision(gs, b, e);
        };
        bool Collision::on_collision(GameState::Type& gs, Bullet& b, Enemy& e) {
            b.kill();
            e.kill();
            gs.add_enemy();
            gs.maybe_add_enemy(0.05);
            gs.add_points(e.score);
            return true;
        };
        bool Collision::on_collision(GameState::Type&, Bullet&, Bullet&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Enemy&, Enemy&) {
            return false;
        };
        void Bullet::out_of_screen(GameState::Type&) {
            this->kill();
        };
        void Enemy::out_of_screen(GameState::Type& gs) {
            this->kill();
            gs.add_enemy();
            gs.maybe_add_enemy(0.02);
            gs.lives--;
            if (gs.lives == 0) gs.game_over = true;
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
                s.player_vel = s.player_vel + ev.vel;
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
        const int speed = 140;
        while(true) {
            SDL_Event ev = sdl_events.pop();
            switch(ev.type) {
                case SDL_QUIT:
                         game_events.push(Events::Type(Events::QuitGame()));
                         return; // we have no more things to do
                         break;
                case SDL_KEYDOWN:
                        if(ev.key.repeat == 0) {
                            switch(ev.key.keysym.sym) {
                                case SDLK_LEFT:  game_events.push(Events::Type(Events::PlayerMove(-speed,0)));
                                                  break;
                                case SDLK_RIGHT: game_events.push(Events::Type(Events::PlayerMove(+speed,0)));

                                                 break;
                            }
                        }
                        switch(ev.key.keysym.sym) {
                            case SDLK_SPACE: game_events.push(Events::Type(Events::Shoot(200)));
                                             break;
                        }
                        break;
                case SDL_KEYUP:
                        switch(ev.key.keysym.sym) {
                            case SDLK_LEFT:  game_events.push(Events::Type(Events::PlayerMove(+speed,0)));
                                             break;
                            case SDLK_RIGHT: game_events.push(Events::Type(Events::PlayerMove(-speed,0)));
                                             break;
                            case SDLK_SPACE: game_events.push(Events::Type(Events::Shoot(200)));
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
                if (s.quit) return;
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

        Game::LSit handler(Game::sdl_info&                   gs,
                                Conc::VarL<GameState::Type>& svar,
                                uint16_t                     fps_relation) {
            return svar.modify([&](GameState::Type& s) {
                if (!s.game_over) apply_velocity(s.player, s.player_vel, s.res, fps_relation);
                // apply background
                SDL_SetRenderDrawColor(gs.win_renderer, 75, 0, 60, 255);
                SDL_RenderClear(gs.win_renderer);
                for(auto p = s.bg_particles.begin(); p != s.bg_particles.end() ;) {
                    gs.with("bg_star", [&](Game::TextureInfo bg_star) {
                        apply_velocity(p->pos, p->vel, s.res, fps_relation);
                        /* this could be optimized such that we don't query the texture more than once
                         *  (per frame, or even `just once` in the whole game)
                         */
                        SDL_Rect r;
                        r.x = p->pos.x;
                        r.y = p->pos.y;
                        r.w = bg_star.width;
                        r.h = bg_star.height;
                        SDL_RenderCopyEx(gs.win_renderer, bg_star.texture, NULL, &r, p->angle, NULL, SDL_FLIP_NONE);
                        p->angle += fps_relation/2;
                    });

                    if (p->pos.y >= s.res.height || p->pos.x <= 0 || p->pos.x >= s.res.width) {
                        p = s.bg_particles.erase(p);
                        s.add_bg_particle();
                    } else {
                        p++;
                    }
                }
                /**/

                if (!s.game_over) {
                    /* draw entities on the world */
                    for (auto curr = s.ent_mp.begin() ; curr != s.ent_mp.end() ;) {
                        if (curr->second->killed) {
                            curr = s.ent_mp.erase(curr);
                        } else {
                            curr++;
                        };
                    };

                    for (auto& curr : s.ent_mp) {
                        gs.with(curr.second->txt_name, [&](Game::TextureInfo text) {
                            apply_velocity(curr.second->pos, curr.second->vel, s.res, fps_relation);
                            SDL_Rect curr_rect;
                            curr_rect.x = curr.second->pos.x;
                            curr_rect.y = curr.second->pos.y;
                            curr_rect.w = text.width;
                            curr_rect.h = text.height;
                            SDL_RenderCopy(gs.win_renderer, text.texture, NULL, &curr_rect);

                            if ((curr.second->pos.x > s.res.width  || curr.second->pos.x < 0) ||
                                (curr.second->pos.y > s.res.height || curr.second->pos.y < 0) ) {
                                    //boost::apply_visitor(Entity::OutOfScreen(s,curr.second), curr.second.var);
                                    curr.second->out_of_screen(s);
                            };
                        });
                    }

                    // collision detection
                    for (auto& curr : s.ent_mp) {
                        gs.with(curr.second->txt_name, [&](Game::TextureInfo text) {
                            SDL_Rect curr_rect;
                            curr_rect.x = curr.second->pos.x;
                            curr_rect.y = curr.second->pos.y;
                            curr_rect.w = text.width;
                            curr_rect.h = text.height;
                            for (auto& other : s.ent_mp) {
                                if (other.first <= curr.first) continue; // we ignore ourselves
                                gs.with(other.second->txt_name, [&](Game::TextureInfo text2) {
                                    SDL_Rect other_rect;
                                    other_rect.x = other.second->pos.x;
                                    other_rect.y = other.second->pos.y;
                                    other_rect.w = text2.width;
                                    other_rect.h = text2.height;
                                    if (collide(&curr_rect, &other_rect)) {
                                        Entity::collision(s, curr.second, other.second);
                                    };
                                });
                            };
                        });
                    };
                    // draw player
                    gs.with("player", [&](Game::TextureInfo player) {
                        SDL_Rect r;
                        r.x = s.player.x;
                        r.y = s.player.y;
                        r.w = player.width;
                        r.h = player.height;
                        SDL_RenderCopy(gs.win_renderer, player.texture, NULL, &r);
                    });
                };

                // hud
                SDL_SetRenderDrawColor(gs.win_renderer, 0, 0, 0, 255);
                {
                    SDL_Rect rect;
                    rect.x = 0;
                    rect.y = s.res.height - 64;
                    rect.w = s.res.width;
                    rect.h = 64;
                    SDL_RenderFillRect(gs.win_renderer, &rect);
                };
                SDL_Color txt_color = {200, 200, 200, 255}; // hud color
                gs.render_text(s.res.width - 256 - 32 , s.res.height - 48, txt_color, "Points:  " + std::to_string(s.points));
                if (!s.game_over) {
                    gs.render_text(48, s.res.height - 48, txt_color, "Lives: ");
                    gs.with("player_life", [&](Game::TextureInfo txt) {
                        SDL_Rect pos;
                        pos.x = 48 + (20 * 7);
                        pos.y = s.res.height - 48;
                        pos.h = txt.height;
                        pos.w = txt.width;
                        for (unsigned int i=0; i < s.lives; i++) {
                            SDL_RenderCopy(gs.win_renderer, txt.texture, NULL, &pos);
                            pos.x += txt.width + 8;
                        };
                    });
                } else {
                    SDL_Color game_over_c = {255, 0, 0, 255};
                    gs.render_text(48, s.res.height - 48, game_over_c, "GAME OVER");
                    SDL_Color game_over_c2 = {0,0,255,255};
                    gs.render_text(s.res.width/3, s.res.height - 48, game_over_c2, "Press space to restart");
                };

                if (s.quit) return Game::BreakLoop;
                return Game::KeepLooping;
            });
        };
    }
}