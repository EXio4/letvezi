#pragma once
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
#include <boost/optional.hpp>
#include "inline_variant.hpp"
#include "timer.h"
#include "vect.h"
#include "persistent.h"
#include "game.h"

namespace Letvezi {
    template <typename T>
    std::shared_ptr<T> inline mk_shared(T&& w) {
        std::shared_ptr<T> x = std::shared_ptr<T>(new T(w));
        x->this_ = x;
        return x;
    };
    struct VelocityT {
        typedef int16_t scalar;
    };
    typedef Vec<VelocityT> Velocity;

    struct PositionT {
        typedef int16_t scalar;
    };
    typedef Vec<PositionT> Position;

    void apply_velocity(Position& pos, Velocity vel, Game::Resolution res, int16_t fps_relation);

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
        struct S_Running;
        struct S_HighScores;
        struct S_Credits;
        struct S_Menu;
        struct S_QuitGame;
        template <typename... Ts>
        using variant = boost::variant<std::shared_ptr<Ts>...>;
        using MState = variant<S_Menu, S_Running, S_HighScores, S_Credits, S_QuitGame>;
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

        Name inline PlayerID() {
            return Name(0);
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

                virtual void extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int) = 0;
                virtual void out_of_screen(std::shared_ptr<GameState::S_Running>) = 0;
        };

        class PowerUp : public Type {
        public:
            enum Type {
                Shield ,
                Bolt   ,
            } kind;
            PowerUp(Type kind) : kind(kind) {};
            void extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int);
            void out_of_screen(std::shared_ptr<GameState::S_Running>);
        };

        class Player : public Type {
            public:
                Player(Position pos_, Velocity vel_) {
                    pos = pos_;
                    vel = vel_;
                    txt_name = "player";
                };
                void extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int);
                void out_of_screen(std::shared_ptr<GameState::S_Running>) {}; // impossible
        };

        class PlayerBullet : public Type {
            public:
                PlayerBullet() {};

                void extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int);
                void out_of_screen(std::shared_ptr<GameState::S_Running>);

        };

        class Enemy : public Type {
            public:
                int score = 100;
                int health = 1;
                int til_next_shoot = -1;
                bool boss = false;
                Enemy(int score, int health, bool boss=false) : score(score), health(health), boss(boss) {};
                Enemy() {};

                void extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int);
                void out_of_screen(std::shared_ptr<GameState::S_Running>);
        };

        class EnemyBullet : public Type {
            public:
                EnemyBullet() {};

                void extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int);
                void out_of_screen(std::shared_ptr<GameState::S_Running>);
        };


        class Collision {
            public:
            bool operator()(std::shared_ptr<GameState::S_Running> gs, std::shared_ptr<Type> x_sp, std::shared_ptr<Type> y_sp) {
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
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , Enemy&          );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , PlayerBullet&   );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , Player&         );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , EnemyBullet&    );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , PowerUp&        );
            // enemy vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&          , Enemy&          );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&          , PlayerBullet&   );
            bool on_collision(std::shared_ptr<GameState::S_Running> g , Enemy&        x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&          , EnemyBullet&    );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&          , PowerUp&        );
            // player_bullet vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running> g , PlayerBullet& x , Enemy&        y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PlayerBullet&   , PlayerBullet&   );
            bool on_collision(std::shared_ptr<GameState::S_Running> g , PlayerBullet& x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PlayerBullet&   , EnemyBullet&    );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PlayerBullet&   , PowerUp&        );
            // enemy_bullet vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running> g , EnemyBullet&  x , Enemy&        y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running> g , EnemyBullet&  x , PlayerBullet& y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running> g , EnemyBullet&  x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running>   , EnemyBullet&    , EnemyBullet&    );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , EnemyBullet&    , PowerUp&        );
            // power up vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running> g , PowerUp&      x , Enemy&        y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running> g , PowerUp&      x , PlayerBullet& y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running> g , PowerUp&      x , Player&       y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running> g , PowerUp&      x , EnemyBullet&  y ) { return on_collision(g,y,x); };
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PowerUp&        , PowerUp&        );
            // fuck up, this is a lot of boilerplate
        };

        extern Collision collision;
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

        Menu MainMenu(); 

        extern int boss_rate;

        struct S_Running {
            Type& parent;
            std::shared_ptr<S_Running> this_;
            struct PlayerInf {
                uint64_t points = 0;
                int32_t  lives  = 100;
                int32_t  shield = 25;
                int32_t  bullet_level = 1;
                int32_t  damage_screen = 0;
            } player;
            int32_t  til_boss      = boss_rate;
            int32_t  bosses_killed = 0;
            boost::optional<TimID> shooting = boost::none;
            std::map<Entity::Name,std::shared_ptr<Entity::Type>> ent_mp;
            Entity::Name last_entity = Entity::Name(1);
            S_Running(Type& parent);
            void add_enemy();
            void maybe_add_enemy(double);
            void add_bullet(Velocity);
            void start_boss(int boss_id);
            void with_new_entity(std::function<std::shared_ptr<Entity::Type>(Entity::Name)> fn);

            void do_damage(unsigned int dm);
            void add_points(uint32_t p);
            void add_shield(unsigned int sh);
            void add_life(unsigned int li);
        };
        struct S_HighScores {
            Type& parent;
            std::shared_ptr<S_HighScores> this_;
            boost::optional<uint64_t> points;
            MState   previous;
            S_HighScores(Type& p, MState prev) : parent(p), points(boost::none), previous(prev) {};
            S_HighScores(Type& p, MState prev, uint64_t points) : parent(p), points(points), previous(prev) {};
        };
        struct S_Credits    {
            Type& parent;
            std::shared_ptr<S_Credits> this_;
            Position text_pos;
            MState   previous;
            S_Credits(Type& parent, MState prev);
        };
        struct S_Menu {
            Type& parent;
            std::shared_ptr<S_Menu> this_;
            Menu menu;
            S_Menu(Type& parent, Menu menu) : parent(parent), menu(menu) {};
        };
        struct S_QuitGame {
            std::shared_ptr<S_QuitGame> this_;
        };

        class Type {
            public:
            struct Common {
                std::shared_ptr<Persistent> persistent;
                std::shared_ptr<Game::sdl_info> sdl_inf;
                struct {
                    std::default_random_engine random_eng;
                    std::uniform_int_distribution<int16_t> i_dis_0_width;
                    std::uniform_int_distribution<int16_t> i_dis_40_100;
                    std::uniform_real_distribution<double> d_dis_0_1;
                } rng;

                Game::Resolution res;
                std::list<Particle> bg_particles;
                struct {
                    Position player_original;
                } cnt;
                Common(std::shared_ptr<Persistent> persistent, std::shared_ptr<Game::sdl_info> sdl_inf, Game::Resolution res, Position player_original) : persistent(persistent), sdl_inf(sdl_inf), res(res), cnt{player_original} {
                    { std::random_device rd;
                        std::default_random_engine r_eg(rd());;
                        std::uniform_int_distribution<int16_t> start_pos(0, res.width); // start positions \x -> (x,0)
                        std::uniform_int_distribution<int16_t> start_speed(40,100); // speed
                        std::uniform_real_distribution<double> enemy_type(0,1); // enemy typ[e
                        rng.random_eng = r_eg;
                        rng.i_dis_0_width = start_pos;
                        rng.i_dis_40_100  = start_speed;
                        rng.d_dis_0_1     = enemy_type;
                    };
                    for (int i=0; i<res.width/20; i++) {
                        add_bg_particle((i*100)%res.height);
                    };
                }
                void add_bg_particle(int16_t y=2);
            } common;

            std::shared_ptr<MState> ms;

            Type(std::shared_ptr<Persistent> persistent, std::shared_ptr<Game::sdl_info> sdl_inf, Game::Resolution res, Position player_original) : common(persistent, sdl_inf, res, player_original), ms (std::make_shared<MState>((mk_shared<S_Menu>(S_Menu(*this, MainMenu()))))) {
            };

            void main_menu() {
                with_menu(MainMenu);
            };
            void with_menu(std::function<Menu()> fn) {
                *ms = menu_ms(fn);
            };
            void start_game() {
                *ms = mk_shared<S_Running>(S_Running(*this));
            };
            void high_scores() {
                *ms = mk_shared<S_HighScores>(S_HighScores(*this, *ms));
            };
            void high_scores(uint64_t points, MState next_ms) {
                *ms = mk_shared<S_HighScores>(S_HighScores(*this, next_ms, points));
            };
            void quit_game() {
                *ms = mk_shared<S_QuitGame>(S_QuitGame());
            };
            void credits() {
                *ms = mk_shared<S_Credits>(S_Credits(*this, *ms));
            };
            MState menu_ms(std::function<Menu()> fn) {
                return mk_shared<S_Menu>(S_Menu(*this, fn()));
            }
            void maybe(double prob, std::function<void()> fn);
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

        class TextInput {
        public:
            std::string str;
            TextInput(char ch) : str(1, ch) {};
            TextInput(std::string str) : str(str) {};
        };

        class Ev {
           private:
              GameState::Type& s;
           public:
              Ev(GameState::Type& s) : s(s) { };
              void operator() () {
                  match(*s.ms,
                        [&](std::shared_ptr<GameState::S_Running> run) {
                            if (run->shooting) {
                                Velocity vel = Velocity(0,-120);
                                vel = (1 + 0.4 * std::min(5,run->player.bullet_level)) * vel;
                                run->add_bullet(vel);
                                run->shooting = s.common.sdl_inf->tim.add_timer(100, *this);
                            } else {
                                run->shooting = boost::none;
                            };
                        },
                        [](std::shared_ptr<GameState::S_HighScores>){},
                        [](std::shared_ptr<GameState::S_QuitGame  >){},
                        [](std::shared_ptr<GameState::S_Menu      >){},
                        [](std::shared_ptr<GameState::S_Credits   >){});
              };
        };

        class ApplyEvent : public boost::static_visitor<void> {
        private:
            GameState::Type& s;
        public:
            ApplyEvent(GameState::Type& s) : s(s) {};
            void operator()(Shoot ev) const;
            void operator()(PlayerMove ev) const;
            void operator()(QuitGame) const;
            void operator()(TextInput txt) const;
            void operator()(EscKey) const;
        };

        struct Type {
            public:
                boost::variant<QuitGame, PlayerMove, Shoot, EscKey, TextInput> data;
            Type(boost::variant<QuitGame, PlayerMove, Shoot, EscKey, TextInput> data) : data(data) {};
        };
    }


    void event_handler(Conc::Chan<SDL_Event>&    sdl_events   ,
                       Conc::Chan<Events::Type>& game_events  );
    void game_handler(Conc::Chan<Events::Type>&     game_events ,
                      Conc::VarL<GameState::Type>&  svar        );

}
