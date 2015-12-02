#include <cstdint>
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <list>
#include <random>
#include <algorithm>
#include <functional>
#include <memory>
#include <boost/variant.hpp>
#include "timer.h"
#include "vect.h"
#include "highscores.h"
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

        Name PlayerID() {
            return Entity::Name(0);
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

                virtual void extra_render(GameState::Type&, Game::sdl_info&, int) = 0;
                virtual void out_of_screen(GameState::Type&) = 0;
        };

        class PowerUp : public Type {
        public:
            enum Type {
                Shield ,
                Bolt   ,
            } kind;
            PowerUp(Type kind) : kind(kind) {};
            void extra_render(GameState::Type&, Game::sdl_info&, int);
            void out_of_screen(GameState::Type&);
        };

        class Player : public Type {
            public:
                int shield = 2500;
                Player(Position pos_, Velocity vel_) {
                    pos = pos_;
                    vel = vel_;
                    txt_name = "player";
                };
                void extra_render(GameState::Type&, Game::sdl_info&, int);
                void out_of_screen(GameState::Type&) {}; // impossible
        };

        class PlayerBullet : public Type {
            public:
                PlayerBullet() {};

                void extra_render(GameState::Type&, Game::sdl_info&, int);
                void out_of_screen(GameState::Type&);

        };

        class Enemy : public Type {
            public:
                int score = 100;
                int health = 1;
                int til_next_shoot = -1;
                bool boss = false;
                Enemy(int score, int health, bool boss=false) : score(score), health(health), boss(boss) {};
                Enemy() {};

                void extra_render(GameState::Type&, Game::sdl_info&, int);
                void out_of_screen(GameState::Type&);
        };

        class EnemyBullet : public Type {
            public:
                EnemyBullet() {};

                void extra_render(GameState::Type&, Game::sdl_info&, int);
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
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<EnemyBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PowerUp*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                } else if (auto x_ = dynamic_cast<PlayerBullet*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<EnemyBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PowerUp*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                } else if (auto x_ = dynamic_cast<Player*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<EnemyBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PowerUp*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                } else if (auto x_ = dynamic_cast<EnemyBullet*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<EnemyBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PowerUp*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                } else if (auto x_ = dynamic_cast<PowerUp*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<EnemyBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PowerUp*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                }
                return false;
            };

            // player vs the world
            bool on_collision(GameState::Type&   , Player&         , Enemy&          );
            bool on_collision(GameState::Type&   , Player&         , PlayerBullet&   );
            bool on_collision(GameState::Type&   , Player&         , Player&         );
            bool on_collision(GameState::Type&   , Player&         , EnemyBullet&    );
            bool on_collision(GameState::Type&   , Player&         , PowerUp&        );
            // enemy vs the world
            bool on_collision(GameState::Type&   , Enemy&          , Enemy&          );
            bool on_collision(GameState::Type&   , Enemy&          , PlayerBullet&   );
            bool on_collision(GameState::Type& g , Enemy&        x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type&   , Enemy&          , EnemyBullet&    );
            bool on_collision(GameState::Type&   , Enemy&          , PowerUp&        );
            // player_bullet vs the world
            bool on_collision(GameState::Type& g , PlayerBullet& x , Enemy&        y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type&   , PlayerBullet&   , PlayerBullet&   );
            bool on_collision(GameState::Type& g , PlayerBullet& x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type&   , PlayerBullet&   , EnemyBullet&    );
            bool on_collision(GameState::Type&   , PlayerBullet&   , PowerUp&        );
            // enemy_bullet vs the world
            bool on_collision(GameState::Type& g , EnemyBullet&  x , Enemy&        y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type& g , EnemyBullet&  x , PlayerBullet& y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type& g , EnemyBullet&  x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type&   , EnemyBullet&    , EnemyBullet&    );
            bool on_collision(GameState::Type&   , EnemyBullet&    , PowerUp&        );
            // power up vs the world
            bool on_collision(GameState::Type& g , PowerUp&      x , Enemy&        y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type& g , PowerUp&      x , PlayerBullet& y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type& g , PowerUp&      x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type& g , PowerUp&      x , EnemyBullet&  y ) { return on_collision(g,y,x); };
            bool on_collision(GameState::Type&   , PowerUp&        , PowerUp&        );
            // fuck up, this is a lot of boilerplate
        } collision;
    };

    namespace GameState {
        struct MenuOption {
            std::string text;
            std::function<void(GameState::Type&)> callback;
        };
        struct Menu {
            std::string title;
            std::vector<MenuOption> opts;
            int16_t current = 0;
            int16_t pressed = 0;
            void move(int16_t x) {
                current = (current + x) % opts.size();
            };
        };

        class EvBoss {
          private:
             Type& s;
             std::shared_ptr<Entity::Type> en;
          public:
            EvBoss(Type& s, std::shared_ptr<Entity::Type> en) : s(s), en(en) {};
            void operator() ();
        };


        int boss_rate = 25000;
        class Type {
            public:
            std::string player_name = "YOU";
            uint64_t       points  = 0;
            int32_t        lives   = 10;
            int            bullet_level = 1;
            int            til_boss = boss_rate;
            int            bosses_killed = 0;
            bool           shooting = false;
            HighScores*    high_scores;

            enum Current {
                Running      ,
                GameOver     ,
                QuitGame     ,
                RestartGame  ,
                GameMenu     ,
                Credits
            } game_state;
            Menu menu;
            Game::Resolution res;
            Position credit_text_pos = Position(32,60);
            Position player_original;
            Game::sdl_info* sdl_inf;
            std::list<Particle> bg_particles;
            struct {
                std::default_random_engine random_eng;
                std::uniform_int_distribution<int16_t> start_pos;
                std::uniform_int_distribution<int16_t> start_speed;
                std::uniform_real_distribution<double> enemy_type;
            } bg_particles_gen;
            std::map<Entity::Name,std::shared_ptr<Entity::Type>> ent_mp;
            Entity::Name last_entity = Entity::Name(1);
            Type(Game::Resolution res_p, Position player_p, Game::sdl_info* sdl_inf, HighScores* high_scores)
               : high_scores(high_scores), res(res_p), player_original(player_p), sdl_inf(sdl_inf) {
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

                    restart_game(true);
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
                    int bullet_rel = std::min(bullet_level * 2, 45);
                    // we don't let enemies spawn too past
                    x = std::min<int16_t>(x, res.width - 100);
                    // we reuse the random number generator of the bg_particles, TODO: rename it
                    if (type > 0.9) {
                        auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(300, 2 + 0.5 * bullet_level));
                        entity->pos = Position(x, 2);
                        entity->vel = Velocity(0,55 + bullet_rel);
                        entity->txt_name = "enemy_3";
                        return entity;
                    } else if (type > 0.4) {
                        auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(200, 1 + 0.35 * bullet_level));
                        entity->pos = Position(x, 2);
                        entity->vel = Velocity(0,50 + bullet_rel);
                        entity->txt_name = "enemy_2";
                        return entity;
                    } else {
                        auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(100, 1 + 0.25 * bullet_level));
                        entity->pos = Position(x, 5);
                        entity->vel = Velocity(0,40 + bullet_rel);
                        entity->txt_name =   "enemy_1";
                        return entity;
                    } 

                });
            };
            void maybe_add_enemy(double prob) {
                double  type  = bg_particles_gen.enemy_type (bg_particles_gen.random_eng);
                if (type > 1-prob) add_enemy();
            };

            void add_bullet(Velocity vel) {
                int mx = std::min<int>(4,bullet_level);
                std::vector<int16_t> mp {
                                    00,
                                    30,
                                    60,
                                    90,
                                    120
                                };
                for (int i=1; i <= mx; i++) {
                    with_new_entity([&](Entity::Name) {
                        std::shared_ptr<Entity::PlayerBullet> entity = std::shared_ptr<Entity::PlayerBullet>(new Entity::PlayerBullet());
                        entity->pos        = ent_mp[Entity::PlayerID()]->pos;
                        entity->pos.x     += mp[i];
                        entity->txt_name   = "player_laser";
                        entity->vel        = vel;
                        return entity;
                    });
                };
                sdl_inf->play_sfx("player_laser");
            }
            void restart_game(bool first=false) {
                Velocity vel = Velocity(0,0);
                if (!first) vel = ent_mp[Entity::PlayerID()]->vel;
                {
                    with_menu([&](Menu& menu) {
                        menu.title = "LETVEZI";
                        menu.opts.push_back(MenuOption{"Start Game",[](GameState::Type& s) {
                                                s.game_state = Running;
                                            }});
                        menu.opts.push_back(MenuOption{"Credits",[](GameState::Type& s) {
                                                s.show_credits();
                                            }});
                        menu.opts.push_back(MenuOption{"Quit Game", [](GameState::Type& s) {
                                                s.quit_game();
                                            }});
                    });
                };
                points = 0;
                lives  = 10;
                bullet_level = 1;
                til_boss = boss_rate;
                bosses_killed = 0;
                ent_mp.clear();
                last_entity = Entity::Name(1);
                ent_mp[Entity::PlayerID()] = std::shared_ptr<Entity::Type>(new Entity::Player(player_original, vel));
                for (int i=0; i<3; i++) {
                    add_enemy();
                };
                maybe_add_enemy(0.5);
                maybe_add_enemy(0.25);
                maybe_add_enemy(0.1);
            };

            void quit_game() {
                game_state = QuitGame;
            };

            void show_credits() {
                game_state = Credits;
                credit_text_pos = Position(res.width/4, res.height + 32);
            };

            void with_menu(std::function<void(Menu&)> fn) {
                game_state = GameMenu;
                menu.current = 0;
                menu.pressed = 0;
                menu.opts.clear();
                fn(menu);
            };

            void start_boss(int boss_id) {
                for (int i=0; i<1+(boss_id/5); i++) {
                    double  p  = bg_particles_gen.enemy_type (bg_particles_gen.random_eng);
                    int pos = res.width / 4 + p * 2 * res.width/4;
                    with_new_entity([&](Entity::Name) {
                        auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(500, 3+2*std::min(5,bullet_level)*boss_id, true));
                        entity->pos = Position(pos, 48);
                        entity->vel = Velocity(40,0);
                        entity->txt_name = "enemy_boss";
                        sdl_inf->tim.add_timer(400, EvBoss(*this, entity));
                        return entity;
                    });
                };
                for (int i=0; i<1+(boss_id * 2); i++) {
                    double p = bg_particles_gen.enemy_type (bg_particles_gen.random_eng);
                    int pos = res.width / 4 + p * 2 * res.width/4;
                    with_new_entity([&](Entity::Name) {
                        auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(500, std::min(5,boss_id), true));
                        entity->pos = Position(pos, 48 + 64);
                        entity->vel = Velocity(20, 100);
                        entity->txt_name = "enemy_boss_squad";
                        sdl_inf->tim.add_timer(400, EvBoss(*this, entity));
                        return entity;
                    });
                };
            };
            void maybe(double prob, std::function<void()> fn) {
                double  p  = bg_particles_gen.enemy_type (bg_particles_gen.random_eng);
                if (p > 1-prob) fn();
            };

            void with_new_entity(std::function<std::shared_ptr<Entity::Type>(Entity::Name)> fn) {
                 Entity::Name e = last_entity;
                 ++last_entity;
                 ent_mp[e] = fn(e);
            }
            void add_points(uint32_t p) {
                points += p;
                til_boss -= p;
                if (til_boss <= 0) {
                    start_boss(1 + bosses_killed);
                    til_boss = (1 + bosses_killed) * boss_rate;
                };
            };
            void add_life(unsigned int li) {
                lives += li;
                if (lives == 0) {
                    high_scores->add_score(player_name, points);
                    game_state = GameState::Type::GameOver;
                };
            };
        };
        void EvBoss::operator() () {
            if (!en->killed) {
                s.with_new_entity([&](Entity::Name) {
                    auto b = std::shared_ptr<Entity::EnemyBullet>(new Entity::EnemyBullet());
                    b->txt_name = "enemy_laser";
                    b->pos = en->pos + Position(55,40);
                    b->vel = Velocity(0, 100);
                    return b;
                });
                s.sdl_inf->tim.add_timer(1000, *this);
            };
        } 
    }

    namespace Entity {
/*
            bool on_collision(GameState::Type&   , Player&         , Enemy&          );
            bool on_collision(GameState::Type&   , Player&         , PlayerBullet&   ); NOP
            bool on_collision(GameState::Type&   , Player&         , Player&         ); NOP
            bool on_collision(GameState::Type&   , Player&         , EnemyBullet&    );
            bool on_collision(GameState::Type&   , Player&         , PowerUp&        );
            // enemy vs the world
            bool on_collision(GameState::Type&   , Enemy&          , Enemy&          );
            bool on_collision(GameState::Type&   , Enemy&      x    , PlayerBullet&   ); 
            bool on_collision(GameState::Type&   , Enemy&          , EnemyBullet&    ); NOP
            bool on_collision(GameState::Type&   , Enemy&          , PowerUp&        ); NOP
            // player_bullet vs the world
            bool on_collision(GameState::Type&   , PlayerBullet&   , PlayerBullet&   ); NOP
            bool on_collision(GameState::Type&   , PlayerBullet&   , EnemyBullet&    );
            bool on_collision(GameState::Type&   , PlayerBullet&   , PowerUp&        );
            // enemy_bullet vs the world
            bool on_collision(GameState::Type&   , EnemyBullet&    , EnemyBullet&    ); NOP
            bool on_collision(GameState::Type&   , EnemyBullet&    , PowerUp&        ); NOP
            // power up vs the world
            bool on_collision(GameState::Type&   , PowerUp&        , PowerUp&        ); NOP
 */

        bool Collision::on_collision(GameState::Type&, Player&, Player&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Player&, PlayerBullet&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Enemy&, Enemy&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Enemy&, EnemyBullet&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Enemy&, PowerUp&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, PlayerBullet&,  PlayerBullet&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, EnemyBullet&,  EnemyBullet&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, EnemyBullet&,  PowerUp&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, PowerUp&,  PowerUp&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, PlayerBullet&,  PowerUp&) {
            return false;
        };

        bool Collision::on_collision(GameState::Type& gs, Player& p, Enemy& e) {
            // TODO: balance tweaks, suggestions for this?
            e.kill();
            if (p.shield <= 5) {
                gs.add_life(-1);
                gs.maybe_add_enemy(0.05);
            };
            gs.add_enemy();
            return true;
        };
        bool Collision::on_collision(GameState::Type& gs, Player& pl, PowerUp& pup) {
            // TODO: powerups
            if (pup.kind == PowerUp::Shield) {
                gs.sdl_inf->play_sfx("shield_enabled");
                pl.shield += 1000;
            } else if (pup.kind == PowerUp::Bolt) {
                gs.bullet_level++;
            }
            pup.kill();
            return true;
        };
        bool Collision::on_collision(GameState::Type& gs, Player& p, EnemyBullet& b) {
            if (!p.shield) {
                gs.add_life(-1);
            };
            b.kill();
            return true;
        };

        bool Collision::on_collision(GameState::Type&, PlayerBullet&, EnemyBullet& b) {
            b.kill();
            return true;
        };

        bool Collision::on_collision(GameState::Type& gs, Enemy& e, PlayerBullet& b) {
            e.health -= ceil(gs.bullet_level);
            b.kill();
            if (e.health <= 0) {
                e.kill();
                if (gs.ent_mp.size() < 50) {
                    gs.add_enemy();
                } else {
                    gs.maybe_add_enemy(0.05);
                };
                gs.maybe_add_enemy(0.05 * 3/(1+gs.bullet_level));
                gs.maybe(0.05 * (1/(1+(gs.bullet_level/6))), [&]() {
                    gs.with_new_entity([&](Entity::Name) {
                        auto p = std::shared_ptr<Entity::PowerUp>(new Entity::PowerUp(Entity::PowerUp::Shield));
                        p->txt_name = "powerup_shield";
                        p->pos = e.pos;
                        p->vel = Velocity(0, 45);
                        return p;
                    });
                });
                gs.maybe(0.1 * (1/(1+(gs.bullet_level/4))), [&]() {
                    gs.with_new_entity([&](Entity::Name) {
                        auto p = std::shared_ptr<Entity::PowerUp>(new Entity::PowerUp(Entity::PowerUp::Bolt));
                        p->txt_name = "powerup_bolt";
                        p->pos = e.pos + Position(0,40);
                        p->vel = Velocity(0, 45);
                        return p;
                    });
                });
                gs.add_points(e.score*1.5);
            };
            gs.add_points(e.score/4);
            return true;
        };

        void PlayerBullet::out_of_screen(GameState::Type&) {
            this->kill();
        };
        void EnemyBullet::out_of_screen(GameState::Type&) {
            this->kill();
        };
        void PowerUp::out_of_screen(GameState::Type&) {
            this->kill();
        };
        void Enemy::out_of_screen(GameState::Type& gs) {
            if (boss) {
                vel = -1 * vel;
            } else {
                this->kill();
                gs.add_enemy();
            }
        };
    }

    namespace Events {
        class QuitGame {
            public:
                QuitGame() { };
        };

        enum PlayerDir {
            Left     ,
            Right    ,
            StopLeft ,
            StopRight,
        };

        class PlayerMove {
            public:
                PlayerDir dir;
                PlayerMove(PlayerDir dir) : dir(dir) {}
        };

        class Shoot {
            public:
                bool key_down = true;

                Shoot(bool key_down)  : key_down(key_down) {
                };
        };
        class EscKey {
            public:
                EscKey() {};
        };

        class Ev {
           private:
              GameState::Type& s;
           public:
              Ev(GameState::Type& s) : s(s) { };
              void operator() () {
                 if (s.game_state == GameState::Type::Running && s.shooting) {
                     Velocity vel = Velocity(0,-120);
                     vel = (1 + 0.4 * std::min(5,s.bullet_level)) * vel;
                     s.add_bullet(vel);
                     s.sdl_inf->tim.add_timer(100, *this);
                 };
              };
        };

        class ApplyEvent : public boost::static_visitor<void> {
        private:
            GameState::Type& s;
        public:
            ApplyEvent(GameState::Type& s) : s(s) {};
            void operator()(Shoot ev) const {
                switch (s.game_state) {
                    case GameState::Type::Running:
                        s.shooting = ev.key_down;
                        {
                           s.sdl_inf->tim.add_timer(0, Ev(s));
                        };
                    break;
                    case GameState::Type::GameOver:
                        if (!ev.key_down) break;
                        s.game_state = GameState::Type::RestartGame;
                    break;
                    case GameState::Type::GameMenu:
                        if (!ev.key_down) break;
                        s.menu.pressed = 150;
                    break;
                    case GameState::Type::Credits:
                        if (!ev.key_down) break;
                        s.game_state = GameState::Type::GameMenu; // we go back to the menu, whatever it was before...
                    case GameState::Type::QuitGame:
                    case GameState::Type::RestartGame:
                        break;
                };
            };
            void operator()(PlayerMove ev) const {
                auto curr_vel = s.ent_mp[Entity::PlayerID()];
                std::vector<int16_t> look {
                    120,
                    140,
                    150,
                    155,
                    160,
                    165
                };
                auto speed = Velocity(look[std::min(6, s.bullet_level)-1], 0);
                auto zero  = Velocity(0,0);
                switch (s.game_state) {
                    case GameState::Type::RestartGame:
                            s.ent_mp[Entity::PlayerID()]->vel = Velocity(0,0);
                        break;
                    case GameState::Type::Running:
                        switch(ev.dir) {
                            case Left:
                                curr_vel->vel = zero-speed;
                                break;
                            case Right:
                                curr_vel->vel = zero+speed;
                                break;
                            case StopLeft:
                                if (curr_vel->vel.x < zero.x)
                                    curr_vel->vel = zero;
                                break;
                            case StopRight:
                                if (curr_vel->vel.x > zero.x)
                                    curr_vel->vel = zero;
                                break;
                        };
                        break;
                    case GameState::Type::GameMenu:
                        if (s.menu.pressed <= 0) {
                            switch(ev.dir) {
                                case Left:
                                    s.menu.move(-1);
                                    break;
                                case Right:
                                    s.menu.move(+1);
                                    break;
                                default:
                                    break;
                            };
                        };
                        break;
                    default:
                        break;
                };
            };
            void operator()(QuitGame) const {
                s.game_state = GameState::Type::QuitGame;
            };
            void operator()(EscKey) const {
                switch (s.game_state) {
                    case GameState::Type::Running:
                        s.with_menu([&](GameState::Menu& menu) {
                            menu.title = "PAUSE";
                            menu.opts.push_back(GameState::MenuOption{"Continue playing", [](GameState::Type& s) {
                                s.game_state = GameState::Type::Running;
                            }});
                            menu.opts.push_back(GameState::MenuOption{"KILL MYSELF", [](GameState::Type& s) {
                                s.add_life(-s.lives);
                            }});
                            menu.opts.push_back(GameState::MenuOption{"Credits",[](GameState::Type& s) {
                                s.show_credits();
                            }});
                            menu.opts.push_back(GameState::MenuOption{"Back to main menu", [](GameState::Type& s) {
                                s.restart_game();
                            }});
                            menu.opts.push_back(GameState::MenuOption{"Quit game", [](GameState::Type& s) {
                                s.quit_game();
                            }});
                        });
                    break;
                    case GameState::Type::GameOver:
                    case GameState::Type::RestartGame:
                    case GameState::Type::QuitGame:
                    case GameState::Type::GameMenu:
                    case GameState::Type::Credits:
                    default:
                    break;
                };
            };
        };

        struct Type {
            public:
                boost::variant<QuitGame, PlayerMove, Shoot, EscKey> data;
            Type(boost::variant<QuitGame, PlayerMove, Shoot, EscKey> data) : data(data) {};
        };
    }


    void event_handler(Conc::Chan<SDL_Event>&    sdl_events ,
                       Conc::Chan<Events::Type>& game_events) {
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
                                case SDLK_LEFT:  game_events.push(Events::Type(Events::PlayerMove(Events::Left)));
                                                 break;
                                case SDLK_RIGHT: game_events.push(Events::Type(Events::PlayerMove(Events::Right)));
                                                 break;
                                case SDLK_ESCAPE: game_events.push(Events::Type(Events::EscKey()));
                                                 break;
                                case SDLK_SPACE: game_events.push(Events::Type(Events::Shoot(true)));
                                                 break;
                            };
                        }
                        break;
                case SDL_KEYUP:
                        switch(ev.key.keysym.sym) {
                            case SDLK_LEFT:  game_events.push(Events::Type(Events::PlayerMove(Events::StopLeft)));
                                             break;
                            case SDLK_RIGHT: game_events.push(Events::Type(Events::PlayerMove(Events::StopRight)));
                                             break;
                            case SDLK_SPACE: game_events.push(Events::Type(Events::Shoot(false)));
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
                if (s.game_state == GameState::Type::QuitGame) return;
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

        struct HudText {
            Position     pos ;
            Game::FontID f_id;
            std::string  text;
            SDL_Color    col ;
        };
        struct HudItem {
            Position pos;
            std::string texture;
        };
        class Hud {
        public:
            int start_hud = 0;
            std::vector<HudText> txts;
            std::vector<HudItem> items;
            Hud() {};
            void add_text(int x, int y, SDL_Color txt_color, std::string text) {
                add_text(Position(x,y), txt_color, text);
            };
            void add_text(Position pos, SDL_Color txt_color, std::string text, Game::FontID f_id=Game::Normal) {
                txts.push_back(HudText{pos,f_id,text,txt_color});
            };
            void add_image(std::string txt_name, Position pos) {
                items.push_back(HudItem{pos,txt_name});
            };
        };

        class Eng { /* this is probably a good fit for inheritance
                       but adding indirections with pointers/reference is boring */
        private:
            Game::sdl_info&    sdl_inf;
            GameState::Type&   s;
            uint16_t           fps_relation;

        public:
            Eng(Game::sdl_info&        gs          ,
                GameState::Type&       s           ,
                uint16_t               fps_relation)
                : sdl_inf(gs), s(s), fps_relation(fps_relation) {};

            Game::LSit render_GameOver();
            Game::LSit render_RestartGame();
            Game::LSit render_Running();
            Game::LSit render_QuitGame();
            Game::LSit render_GameMenu();
            Game::LSit render_Credits();

            void render_pic(Position, std::string,uint8_t alpha=255);
            void render_background();
            void render_hud(const Hud&);
        };

        Game::LSit Eng::render_QuitGame() {
            return Game::BreakLoop;
        };
        Game::LSit Eng::render_RestartGame() {
            s.restart_game();
            return render_GameMenu();
        };
        Game::LSit Eng::render_Running() {

            std::shared_ptr<Entity::Type> player_pointer = s.ent_mp[Entity::PlayerID()];

            /* draw entities on the world */
            for (auto curr = s.ent_mp.begin() ; curr != s.ent_mp.end() ;) {
                if (curr->second->killed) {
                    curr = s.ent_mp.erase(curr);
                } else {
                    curr++;
                };
            };

            for (auto& curr : s.ent_mp) {
                apply_velocity(curr.second->pos, curr.second->vel, s.res, fps_relation);
                render_pic(curr.second->pos, curr.second->txt_name);
                curr.second->extra_render(s, sdl_inf, fps_relation);
                if ((curr.second->pos.x > s.res.width  || curr.second->pos.x < 0) ||
                    (curr.second->pos.y > s.res.height || curr.second->pos.y < 0) ) {
                        curr.second->out_of_screen(s);
                };
            }

            // collision detection
            for (auto& curr : s.ent_mp) {
                if (curr.second->killed) continue;
                sdl_inf.with(curr.second->txt_name, [&](Game::TextureInfo text) {
                    SDL_Rect curr_rect { curr.second->pos.x, curr.second->pos.y , text.width, text.height };
                    for (auto& other : s.ent_mp) {
                        if (other.first <= curr.first) continue; // we ignore ourselves (and previous checks)
                        if (other.second->killed)      continue; // we ignore dead entities
                        if (curr.second->killed)       break;    // ^ and this are used for avoiding extra `duplication`
                               // x when more than entity collide with the same enemy or so
                        sdl_inf.with(other.second->txt_name, [&](Game::TextureInfo text2) {
                            SDL_Rect other_rect { other.second->pos.x, other.second->pos.y , text2.width , text2.height }; 
                            if (collide(&curr_rect, &other_rect)) {
                                Entity::collision(s, curr.second, other.second);
                            };
                        });
                    };
                });
            };

            {
                Hud hud;
                SDL_Color txt_color = {200, 200, 200, 255}; // hud color
                hud.start_hud = s.res.height - 64;
                hud.add_text(s.res.width - 256 - 32 , s.res.height - 48, txt_color, "Points:  " + std::to_string(s.points));
                hud.add_text(48, s.res.height - 48, txt_color, "Lives: ");
                Position pos(48 + 20 * 7, s.res.height - 48);
                for (int32_t i=0; i < s.lives; i++) {
                    hud.add_image("player_life", pos);
                    pos.x += 40;
                };

                render_hud(hud);
            };

            return Game::KeepLooping;
        };

        Game::LSit Eng::render_GameOver() {
            {
                Hud hud;
                {
                    SDL_Color player_name {200,   0, 200, 255};
                    SDL_Color points      {200, 200, 200, 255};
                    hud.add_text(Position(48, 64+16), points, "High scores");
                    Position pos(64, 64+48);
                    for (auto& r : s.high_scores->table()) {
                        hud.add_text(pos, player_name , r.name);
                        hud.add_text(pos + Position(s.res.width/3, 0) , points, std::to_string(r.points));
                        pos += Position(0,48);
                    };
                };
                SDL_Color txt_color {200, 200, 200, 255}; // hud color
                hud.start_hud = s.res.height - 64;
                hud.add_text(s.res.width - 256 - 32 , s.res.height - 48, txt_color, "Points:  " + std::to_string(s.points));
                SDL_Color game_over_c {255, 0, 0, 255};
                hud.add_text(48, s.res.height - 100, game_over_c, "GAME OVER");
                SDL_Color game_over_c2 = {0,0,255,255};
                hud.add_text(s.res.width/3, s.res.height - 48, game_over_c2, "Press space to go back to the main menu");
                render_hud(hud);
            };
            return Game::KeepLooping;
        };

        Game::LSit Eng::render_Credits() {
            Position pos = s.credit_text_pos;
            Hud      hud;
            SDL_Color txt_color = {200, 200, 200, 255};
            if (pos.y < -200) s.game_state = GameState::Type::GameMenu;
            hud.start_hud = 64;
            std::vector<std::pair<Game::FontID,std::string>> entries {
                {Game::Huge  , "Credits"                         },
                {Game::Normal, ""                                },
                {Game::Huge  , "LETVEZI"                         },
                {Game::Huge  , "Programming work and design"     },
                {Game::Normal, "      Name1"                     },
                {Game::Normal, "      Name2"                     },
                {Game::Normal, "      Name3"                     },
                {Game::Normal, ""                                },
                {Game::Normal, "Art"                             },
                {Game::Normal, "      Kenney  http://kenney.nl/" },
                {Game::Normal, "      neocrey http://neocrey.com"}
            };
            for (auto& i : entries) {
                hud.add_text(pos, txt_color, i.second, i.first);
                if (pos.x >= s.res.width || pos.y >= s.res.height) break;
                Position rel = Position(0,0);
                switch(i.first) {
                    case Game::Small:
                            rel = Position(0,64);
                        break;
                    case Game::Normal:
                            rel = Position(0,96);
                        break;
                    case Game::Huge:
                            rel = Position(0,128);
                        break;
                };
                pos += rel;
            };
            render_hud(hud);
            apply_velocity(s.credit_text_pos, Velocity(0, -30), s.res, fps_relation);
            return Game::KeepLooping;
        };

        Game::LSit Eng::render_GameMenu() {
            {
                Hud hud;
                SDL_Color txt_color = {200, 200, 200, 255}; // hud color
                hud.start_hud = s.res.height - (256+128);;
                hud.add_text(Position(48, hud.start_hud + 32), txt_color, s.menu.title, Game::Huge);
                SDL_Color pressed_option {255, 0,   0, 255};
                SDL_Color current_option {255, 0, 255, 255};
                SDL_Color other_option   {0,   0, 255, 255};
                // TODO: see how we could deal with the menu w/o using a normal loop
                // and still have an easy way to select the next/prev menu entry easily
                Position pos(128, hud.start_hud + 128); // start pos
                for (uint16_t i=0; i<s.menu.opts.size(); i++) {
                    auto c = s.menu.opts[i];
                    if (i == s.menu.current) {
                        if (s.menu.pressed > 0) {
                            hud.add_text(pos, pressed_option, c.text);
                            s.menu.pressed -= fps_relation;
                            if (s.menu.pressed <= 0) {
                                s.menu.pressed = 0;
                                c.callback(s);
                            };
                        } else {
                            hud.add_text(pos, current_option, c.text);
                        }
                    } else {
                            hud.add_text(pos, other_option,   c.text);
                    };
                    pos += Position(c.text.length() * (16+8) ,0);
                };
                hud.add_text(Position(s.res.width - (512+256) , hud.start_hud + 256     ), pressed_option, "Left/Right keys = Move between options");
                hud.add_text(Position(s.res.width - (512+256) , hud.start_hud + 256 + 32), pressed_option, "Space key       = Select current option");
                render_hud(hud);
            };
            return Game::KeepLooping;
        };

        void Eng::render_pic(Position pos, std::string txt_name, uint8_t alpha) {
            sdl_inf.with(txt_name, [&](Game::TextureInfo text) {
                SDL_Rect r;
                r.x = pos.x;
                r.y = pos.y;
                r.w = text.width;
                r.h = text.height;
                SDL_SetTextureAlphaMod(text.texture, alpha);
                SDL_RenderCopy(sdl_inf.win_renderer, text.texture, NULL, &r);
            });
        };

        void Eng::render_hud(const Hud& hud) {
                // hud
                SDL_SetRenderDrawColor(sdl_inf.win_renderer, 0, 0, 0, 255);
                {
                    SDL_Rect rect {0, hud.start_hud, s.res.width, s.res.height - hud.start_hud};
                    SDL_RenderFillRect(sdl_inf.win_renderer, &rect);
                };
                for (auto& txt : hud.txts) {
                    sdl_inf.render_text(txt.pos.x, txt.pos.y, txt.f_id, txt.col, txt.text);
                };
                for (auto& spr : hud.items) {
                    render_pic(Position(spr.pos.x, spr.pos.y), spr.texture);
                };
        };

        void Eng::render_background() {
                // apply background
                SDL_SetRenderDrawColor(sdl_inf.win_renderer, 75, 0, 60, 255);
                SDL_RenderClear(sdl_inf.win_renderer);

                sdl_inf.with("bg_star", [&](Game::TextureInfo bg_star) {
                    for(auto p = s.bg_particles.begin(); p != s.bg_particles.end() ;) {
                        apply_velocity(p->pos, p->vel, s.res, fps_relation);
                        SDL_Rect r {p->pos.x, p->pos.y, bg_star.width, bg_star.height};
                        SDL_RenderCopyEx(sdl_inf.win_renderer, bg_star.texture, NULL, &r, p->angle, NULL, SDL_FLIP_NONE);
                        p->angle += fps_relation/2;
                        if (p->pos.y >= s.res.height || p->pos.x <= 0 || p->pos.x >= s.res.width) {
                            p = s.bg_particles.erase(p);
                            s.add_bg_particle();
                        } else {
                            p++;
                        }
                    }
                });

                /**/
        }

        Game::LSit handler_game(Game::sdl_info&              gs,
                                Conc::VarL<GameState::Type>& svar,
                                uint16_t                     fps_relation) {
            return svar.modify([&](GameState::Type& s) {
                Eng eng = Eng(gs, s, fps_relation);
                eng.render_background();
                gs.tim.advance(fps_relation);
                switch (s.game_state) {
                    case GameState::Type::GameOver:
                        return eng.render_GameOver();
                        break;
                    case GameState::Type::RestartGame:
                        return eng.render_RestartGame();
                        break;
                    case GameState::Type::Running:
                        return eng.render_Running();
                        break;
                    case GameState::Type::QuitGame:
                        return eng.render_QuitGame();
                        break;
                    case GameState::Type::GameMenu:
                        return eng.render_GameMenu();
                        break;
                    case GameState::Type::Credits:
                        return eng.render_Credits();
                        break;
                }
            });
        };
    };
    namespace Entity {

        void Player::extra_render(GameState::Type& gs, Game::sdl_info& sdl_inf, int fps_rel) {
            if (shield > 0) {
                Render::Eng eng(sdl_inf, gs, fps_rel);
                int alpha = 255;
                if (shield < 1000) alpha = shield/4;
                eng.render_pic(pos - Position(10,16), "player_shield", alpha);
                shield -= fps_rel;
            };
        };
        void PlayerBullet::extra_render(GameState::Type&, Game::sdl_info&, int) {
        };
        void EnemyBullet::extra_render(GameState::Type&, Game::sdl_info&, int) {
        };
        void PowerUp::extra_render(GameState::Type&, Game::sdl_info&, int) {
        };
        void Enemy::extra_render(GameState::Type&, Game::sdl_info&, int) {
        };
    }

}
