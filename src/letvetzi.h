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
#include "timer.h"
#include "vect.h"
#include "persistent.h"
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

        Name PlayerID();

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
                int32_t shield = 2500;
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
        };
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


        extern int boss_rate;
        class Type {
            public:
            uint64_t       points  = 0;
            int32_t        lives   = 10;
            int            bullet_level = 1;
            int            til_boss = boss_rate;
            int            bosses_killed = 0;
            bool           shooting = false;
            Persistent*    persistent_data;

            enum Current {
                Running      ,
                HighScores   ,
                QuitGame     ,
//                BackToMMenu,
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
            Type(Game::Resolution res_p, Position player_p, Game::sdl_info* sdl_inf, Persistent* persistent);

            void add_bg_particle(int16_t start_y=0);
            void add_enemy();
            void maybe_add_enemy(double prob);

            void add_bullet(Velocity vel);

            void restart_game(bool first=false);
            void quit_game();
            void show_highscores();
            void back_to_main_menu();
            void show_credits();

            void with_menu(std::function<void(Menu&)> fn);
            void start_boss(int boss_id);
            void maybe(double prob, std::function<void()> fn);
            void with_new_entity(std::function<std::shared_ptr<Entity::Type>(Entity::Name)> fn);
            void add_points(uint32_t p);
            void add_life(unsigned int li);
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
    /* fps_relation, 1000 = 1s
     * 
     * the speed unit used internally, is defined as `1u = 1% of the screen` approx.
     */
    namespace Render {
        bool collide(const SDL_Rect *r1, const SDL_Rect *r2);

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

            Game::LSit render_HighScores();
            Game::LSit render_Running();
            Game::LSit render_QuitGame();
            Game::LSit render_GameMenu();
            Game::LSit render_Credits();

            void render_pic(Position, std::string,uint8_t alpha=255);
            void render_background();
            void render_hud(const Hud&);
        };

        Game::LSit handler_game(Game::sdl_info&              gs,
                                Conc::VarL<GameState::Type>& svar,
                                uint16_t                     fps_relation);
    };

}
