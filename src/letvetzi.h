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
                virtual void out_of_screen(GameState::Type&) = 0;
        };

        class Player : public Type {
            public:
                Player(Position pos_, Velocity vel_) {
                    pos = pos_;
                    vel = vel_;
                    txt_name = "player";
                };
                void out_of_screen(GameState::Type&) {}; // impossible
        };

        class PlayerBullet : public Type {
            public:
                PlayerBullet() {};
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
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                } else if (auto x_ = dynamic_cast<PlayerBullet*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                } else if (auto x_ = dynamic_cast<Player*>(x)) {
                    if (auto y_ = dynamic_cast<Enemy*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<PlayerBullet*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    } else if (auto y_ = dynamic_cast<Player*>(y)) {
                        return on_collision(gs, *x_, *y_);
                    };
                }
                return false;
            };

            bool on_collision(GameState::Type&, Enemy&        , Enemy&       );
            bool on_collision(GameState::Type&, Enemy&        , PlayerBullet&);
            bool on_collision(GameState::Type&, Enemy&        , Player&      );
            bool on_collision(GameState::Type&, PlayerBullet& , Enemy&       );
            bool on_collision(GameState::Type&, PlayerBullet& , PlayerBullet&);
            bool on_collision(GameState::Type&, PlayerBullet& , Player&      );
            bool on_collision(GameState::Type&, Player&       , Enemy&       );
            bool on_collision(GameState::Type&, Player&       , PlayerBullet&);
            bool on_collision(GameState::Type&, Player&       , Player&      );

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
        class Type {
            public:
            uint64_t       points  = 0;
            unsigned int   lives   = 10;
            enum Current {
                Running      ,
                GameOver     ,
                QuitGame     ,
                RestartGame  ,
                GameMenu
            } game_state;
            Menu menu;
            Game::Resolution res;
            Position player_original;
            std::list<Particle> bg_particles;
            struct {
                std::default_random_engine random_eng;
                std::uniform_int_distribution<int16_t> start_pos;
                std::uniform_int_distribution<int16_t> start_speed;
                std::uniform_real_distribution<double> enemy_type;
            } bg_particles_gen;
            std::map<Entity::Name,std::shared_ptr<Entity::Type>> ent_mp;
            Entity::Name last_entity = Entity::Name(1);
            Type(Game::Resolution res_p, Position player_p)
               : res(res_p), player_original(player_p) {
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
                    // we don't let enemies spawn too past
                    x = std::min<int16_t>(x, res.width - 100);
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
                with_new_entity([&](Entity::Name) {
                    Entity::PlayerBullet *entity = new Entity::PlayerBullet();
                    entity->pos        = ent_mp[Entity::PlayerID()]->pos;
                    entity->pos.x     += 46;
                    entity->txt_name   = "player_laser";
                    entity->vel        = vel;
                    return entity;
                });
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
                        menu.opts.push_back(MenuOption{"Quit Game", [](GameState::Type& s) {
                                                s.game_state = QuitGame;
                                            }});
                    });
                };
                points = 0;
                lives  = 10;
                ent_mp.clear();
                last_entity = Entity::Name(1);
                ent_mp[Entity::PlayerID()] = std::shared_ptr<Entity::Type>(new Entity::Player(player_original, vel));
                for (int i=0; i<5; i++) {
                    add_enemy();
                };
            };

            void with_menu(std::function<void(Menu&)> fn) {
                game_state = GameMenu;
                menu.current = 0;
                menu.pressed = 0;
                menu.opts.clear();
                fn(menu);
            };

            void with_new_entity(std::function<Entity::Type*(Entity::Name)> fn) {
                 Entity::Name e = last_entity;
                 ++last_entity;
                 ent_mp[e] = std::shared_ptr<Entity::Type>(fn(e));
            }
            void add_points(uint32_t p) {
                points += p;
            };
        };
    }

    namespace Entity {
        bool Collision::on_collision(GameState::Type& gs, Enemy& e, PlayerBullet& b) {
                    return Collision::on_collision(gs, b, e);
        };
        bool Collision::on_collision(GameState::Type& gs, PlayerBullet& b, Enemy& e) {
            b.kill();
            e.kill();
            gs.add_enemy();
            gs.maybe_add_enemy(0.05);
            gs.add_points(e.score);
            return true;
        };
        bool Collision::on_collision(GameState::Type& gs, Player&, Enemy& e) {
            // TODO: balance tweaks, suggestions for this?
            e.kill();
            gs.add_enemy();
            gs.add_enemy();
            return true;
        };
        bool Collision::on_collision(GameState::Type& gs, Enemy& e, Player& p) {
            return on_collision(gs, p, e);
        };
        bool Collision::on_collision(GameState::Type&, PlayerBullet&, Player&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Player&, PlayerBullet&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, PlayerBullet&, PlayerBullet&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Enemy&, Enemy&) {
            return false;
        };
        bool Collision::on_collision(GameState::Type&, Player&, Player&) {
            return false;
        };
        void PlayerBullet::out_of_screen(GameState::Type&) {
            this->kill();
        };
        void Enemy::out_of_screen(GameState::Type& gs) {
            this->kill();
            gs.add_enemy();
            gs.maybe_add_enemy(0.02);
            gs.lives--;
            if (gs.lives == 0) gs.game_state = GameState::Type::GameOver;
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
                Velocity vel;

                Shoot(int16_t accel)  : vel(Velocity(0,-accel)) {
                };
        };
        class EscKey {
            public:
                EscKey() {};
        };

        class ApplyEvent : public boost::static_visitor<void> {
        private:
            GameState::Type& s;
        public:
            ApplyEvent(GameState::Type& s) : s(s) {};
            void operator()(Shoot ev) const {
                switch (s.game_state) {
                    case GameState::Type::Running:
                        s.add_bullet(ev.vel);
                    break;
                    case GameState::Type::GameOver:
                        s.game_state = GameState::Type::RestartGame;
                    break;
                    case GameState::Type::GameMenu:
                        s.menu.pressed = 150;
                    break;
                    case GameState::Type::QuitGame:
                    case GameState::Type::RestartGame:
                        break;
                };
            };
            void operator()(PlayerMove ev) const {
                auto curr_vel = s.ent_mp[Entity::PlayerID()];
                auto speed = Velocity(140, 0);
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
                            menu.opts.push_back(GameState::MenuOption{"Quit game", [](GameState::Type& s) {
                                s.game_state = GameState::Type::QuitGame;
                            }});
                        });
                    break;
                    case GameState::Type::GameOver:
                    case GameState::Type::RestartGame:
                    case GameState::Type::QuitGame:
                    case GameState::Type::GameMenu:
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
                            }
                        }
                        switch(ev.key.keysym.sym) {
                            case SDLK_SPACE: game_events.push(Events::Type(Events::Shoot(200)));
                                             break;
                        }
                        break;
                case SDL_KEYUP:
                        switch(ev.key.keysym.sym) {
                            case SDLK_LEFT:  game_events.push(Events::Type(Events::PlayerMove(Events::StopLeft)));
                                             break;
                            case SDLK_RIGHT: game_events.push(Events::Type(Events::PlayerMove(Events::StopRight)));
                                             break;
                            /*case SDLK_SPACE: game_events.push(Events::Type(Events::Shoot(200)));
                                             break; */
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

            void render_pic(Position, std::string);
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
                if ((curr.second->pos.x > s.res.width  || curr.second->pos.x < 0) ||
                    (curr.second->pos.y > s.res.height || curr.second->pos.y < 0) ) {
                        curr.second->out_of_screen(s);
                };
            }

            // collision detection
            for (auto& curr : s.ent_mp) {
                sdl_inf.with(curr.second->txt_name, [&](Game::TextureInfo text) {
                    SDL_Rect curr_rect { curr.second->pos.x, curr.second->pos.y , text.width, text.height };
                    for (auto& other : s.ent_mp) {
                        if (other.first <= curr.first) continue; // we ignore ourselves (and previous checks)   
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
                for (unsigned int i=0; i < s.lives; i++) {
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
                SDL_Color txt_color = {200, 200, 200, 255}; // hud color
                hud.start_hud = s.res.height - 128;
                hud.add_text(s.res.width - 256 - 32 , s.res.height - 48, txt_color, "Points:  " + std::to_string(s.points));
                SDL_Color game_over_c = {255, 0, 0, 255};
                hud.add_text(48, s.res.height - 100, game_over_c, "GAME OVER");
                SDL_Color game_over_c2 = {0,0,255,255};
                hud.add_text(s.res.width/3, s.res.height - 48, game_over_c2, "Press space to restart");
                render_hud(hud);
            };
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
                Position pos(32, hud.start_hud + 128); // start pos
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
                    pos += Position(c.text.length() * 32 ,0);
                };
                render_hud(hud);
            };
            return Game::KeepLooping;
        };

        void Eng::render_pic(Position pos, std::string txt_name) {
            sdl_inf.with(txt_name, [&](Game::TextureInfo text) {
                SDL_Rect r;
                r.x = pos.x;
                r.y = pos.y;
                r.w = text.width;
                r.h = text.height;
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

        Game::LSit handler_game(Game::sdl_info&                   gs,
                                Conc::VarL<GameState::Type>& svar,
                                uint16_t                     fps_relation) {
            return svar.modify([&](GameState::Type& s) {
                Eng eng = Eng(gs, s, fps_relation);
                eng.render_background();
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
                    default:
                        break;
                }
            });
        };
    }
}