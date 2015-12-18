#include "letvezi.h"
namespace Letvezi {
    void apply_velocity(Position& pos, Velocity vel, Game::Resolution res, int16_t fps_relation) {
        int16_t p_x = pos.x + (vel.x * res.width  * fps_relation) /100/1000;
        int16_t p_y = pos.y + (vel.y * res.height * fps_relation) /100/1000;
        pos.x = std::max<int16_t>(-64, std::min<int16_t>(p_x, res.width+64));
        pos.y = std::max<int16_t>(-64, std::min<int16_t>(p_y, res.height+64));
    };

    namespace Events {
            void ApplyEvent::operator()(Shoot ev) const {
                (void)ev;
                match(*s.ms,
                        [&](std::shared_ptr<GameState::S_Running> run) {
                            if (ev.key_down && !run->shooting) {
                                run->shooting = s.common.sdl_inf->tim.add_timer(0,Ev(s));
                            } else if (!ev.key_down && run->shooting) {
                                s.common.sdl_inf->tim.remove(*run->shooting);
                                run->shooting = boost::none;
                            };
                        },
                        [&](std::shared_ptr<GameState::S_HighScores>) {
                        },
                        [&](std::shared_ptr<GameState::S_Menu> menu) {
                            if (!ev.key_down) return;
                            menu->menu.pressed = 150;
                        },
                        [&](std::shared_ptr<GameState::S_Credits> credits) {
                            if (!ev.key_down) return;
                            *s.ms = credits->previous;
                        },
                        [&](std::shared_ptr<GameState::S_QuitGame>) {
                        });
            };
            void ApplyEvent::operator()(PlayerMove ev) const {
                match(*s.ms,
                        [&](std::shared_ptr<GameState::S_Running> run) {
                            auto curr_vel = run->ent_mp[Entity::PlayerID()];
                            std::vector<int16_t> look {
                                120,
                                140,
                                150,
                                155,
                                160,
                                165
                            };
                            auto speed = Velocity(look[std::min(6, run->player.bullet_level)-1], 0);
                            speed.x = std::min<double>(speed.x, std::max<double>(0.5 * (double)speed.x, (double)speed.x * ((double)run->player.health/75)));
                            auto zero  = Velocity(0,0);
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
                        },
                        [&](std::shared_ptr<GameState::S_Menu> menu) {
                            if (menu->menu.pressed <= 0) {
                                switch(ev.dir) {
                                    case Left:
                                        menu->menu.move(-1);
                                        break;
                                    case Right:
                                        menu->menu.move(+1);
                                        break;
                                    default:
                                        break;
                                };
                            };
                        },
                        [&](std::shared_ptr<GameState::S_HighScores>) {
                        },
                        [&](std::shared_ptr<GameState::S_Credits>) {
                        },
                        [&](std::shared_ptr<GameState::S_QuitGame>) {
                        });
            };
            void ApplyEvent::operator()(QuitGame) const {
                s.quit_game();
            };
            void ApplyEvent::operator()(TextInput txt) const {
                if (txt.str.size() == 0) return;
                match(*s.ms,
                        [&](std::shared_ptr<GameState::S_HighScores> hs) {
                            if (txt.str[0] == '\n') {
                                *s.ms = hs->previous;
                            } else if (hs->points) {
                                return;
                            } else if (txt.str[0] == '\b') {
                                if (hs->parent.common.persistent->user_data.player_name.size() > 0) {
                                    hs->parent.common.persistent->user_data.player_name.pop_back();
                                };
                            } else {
                                hs->parent.common.persistent->user_data.player_name += txt.str;
                            };
                        },
                        [&](std::shared_ptr<GameState::S_Running>) {
                        },
                        [&](std::shared_ptr<GameState::S_Menu>) {
                        },
                        [&](std::shared_ptr<GameState::S_Credits>) {
                        },
                        [&](std::shared_ptr<GameState::S_QuitGame>) {
                        });
            };
            void ApplyEvent::operator()(EscKey) const {
                    match(*s.ms,
                        [](std::shared_ptr<GameState::S_Running> run) {
                            run->parent.with_menu([run]() {
                                GameState::Menu menu;
                                menu.title = "PAUSE";
                                    menu.opts.push_back(GameState::MenuOption{"Continue playing", [run](GameState::Type& s) {
                                    *s.ms = run;
                                }});
                                menu.opts.push_back(GameState::MenuOption{"KILL MYSELF", [run](GameState::Type&) {
                                    run->add_life(-run->player.health);
                                }});
                                menu.opts.push_back(GameState::MenuOption{"Credits",[](GameState::Type& s) {
                                    s.credits();
                                }});
                                menu.opts.push_back(GameState::MenuOption{"Back to main menu", [](GameState::Type& s) {
                                    s.main_menu();
                                }});
                                menu.opts.push_back(GameState::MenuOption{"Quit game", [](GameState::Type& s) {
                                    s.quit_game();
                                }});
                                return menu;
                            });
                        },
                        [&](std::shared_ptr<GameState::S_HighScores>) {
                        },
                        [&](std::shared_ptr<GameState::S_Menu>) {
                        },
                        [&](std::shared_ptr<GameState::S_Credits>) {
                        },
                        [&](std::shared_ptr<GameState::S_QuitGame>) {
                        });
            };
    };

    namespace GameState {
        Menu MainMenu() {
            Menu menu;
            menu.title = "LETVEZI";
            menu.opts.push_back(MenuOption{"Start Game",[](GameState::Type& s) {
                                    s.start_game();
                                }});
            menu.opts.push_back(MenuOption{"High scores", [](GameState::Type& s) {
                                    s.high_scores();
                                }});
            menu.opts.push_back(MenuOption{"Credits",[](GameState::Type& s) {
                                    s.credits();
                                }});
            menu.opts.push_back(MenuOption{"Quit Game", [](GameState::Type& s) {
                                    s.quit_game();
                                }});
            return menu;
        };
        class EnemySh {
          private:
             std::shared_ptr<S_Running> run;
             std::shared_ptr<Entity::Type> en;
             Game::TextureID txt_name;
             uint16_t speed;
          public:
            EnemySh(std::shared_ptr<S_Running> run, std::shared_ptr<Entity::Type> en, Game::TextureID txt_name = Game::TEX_EnemyBossLaser, uint16_t speed=150) : run(run), en(en), txt_name(txt_name), speed(speed) {
            };
            void operator() () {
                if (!en->killed) {
                    run->with_new_entity([&](Entity::Name) {
                        auto b = std::shared_ptr<Entity::EnemyBullet>(new Entity::EnemyBullet(txt_name));
                        b->pos = en->pos + Position(55,40);
                        b->vel = Velocity(0, speed);
                        return b;
                    });
                    double x = run->parent.common.rng.d_dis_0_1(run->parent.common.rng.random_eng);
                    run->parent.common.sdl_inf->tim.add_timer(650 + (x * 450), *this);
                };
            };
        };

        int boss_rate = 25000;
        void Type::Common::add_bg_particle(int16_t start_y) {
            int16_t x     = rng.i_dis_0_width (rng.random_eng);
            int16_t speed = rng.i_dis_40_100  (rng.random_eng);
            int16_t ysped = rng.i_dis_40_100  (rng.random_eng);
            bg_particles.push_front(Particle(Game::TEX_BackgroundStar, Position(Vec<PositionT>(x,start_y)), Velocity(ysped-70, speed/2)));
        };
        void Type::maybe(double prob, std::function<void()> fn) {
            double  p  = common.rng.d_dis_0_1 (common.rng.random_eng);
            if (p > 1-prob) fn();
        };

        S_Running::S_Running(Type& parent) : parent(parent) {
            ent_mp[Entity::PlayerID()] = std::shared_ptr<Entity::Type>(new Entity::Player(parent.common.cnt.player_original, Velocity(0,0)));
            for (int i=0; i<3; i++) {
                add_enemy(false);
            };
            maybe_add_enemy(0.75, false);
            maybe_add_enemy(0.5 , false);
            maybe_add_enemy(0.25, false);
            maybe_add_enemy(0.1 , false);
        };
        S_Credits::S_Credits(Type& parent, MState prev) : parent(parent),
                    text_pos(Position(parent.common.res.width/4, parent.common.res.height + 32)),
                    previous(prev) {};

        void S_Running::add_bullet(Velocity vel) {
            int mx = std::min<int>(4,player.bullet_level);
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
                    entity->vel        = vel;
                    return entity;
                });
            };
            parent.common.sdl_inf->play_sfx(Game::SFX_PlayerLaser);
        }
        void S_Running::add_points(uint32_t p) {
            player.points += p;
            til_boss -= p;
            if (til_boss <= 0) {
                start_boss(1 + bosses_killed);
                til_boss = (1 + bosses_killed) * boss_rate;
            };
        };
        void S_Running::add_life(unsigned int li) {
            player.health += li;
            if (player.health <= 0) {
                parent.common.persistent->high_scores.add_score(parent.common.persistent->user_data.player_name, player.points);
                parent.high_scores(player.points, parent.menu_ms(MainMenu));
            };
            player.health = std::min(player.health, 125);
        };
        void S_Running::do_damage(unsigned int dm) {
            if (player.damage_screen > 0) return;
            if (player.shield >= 0) {
                add_shield(dm * -2);
            } else {
                add_life(dm * -1);
            }
            player.damage_screen = 100;
        };
        void S_Running::add_shield(unsigned int sh) {
            player.shield += sh;
            if (player.shield < 0) {
                auto d = player.shield;
                player.shield = 0;
                add_life(d);
            } else if (player.shield > 100) {
                add_life((player.shield-100)/5);
                player.shield = 100;
            }
        };
        void S_Running::start_boss(int boss_id) {
            for (int i=0; i<1+boss_id; i++) {
                double  p  = parent.common.rng.d_dis_0_1 (parent.common.rng.random_eng);
                int    pos = parent.common.res.width / 4 + p * 2 * parent.common.res.width/4;
                with_new_entity([&](Entity::Name) {
                    auto entity = std::make_shared<Entity::Enemy>(Game::TEX_EnemyBoss, 500, 3+2*std::min(5,player.bullet_level)*boss_id, true);
                    entity->pos = Position(pos, 48);
                    entity->vel = Velocity(40,0);
                    parent.common.sdl_inf->tim.add_timer(400, EnemySh(this_, entity));
                    return entity;
                });
            };
            for (int i=0; i<1+(boss_id * 2); i++) {
                double  p  = parent.common.rng.d_dis_0_1 (parent.common.rng.random_eng);
                int pos = parent.common.res.width / 4 + p * 2 * parent.common.res.width/4;
                with_new_entity([&](Entity::Name) {
                    auto entity = std::make_shared<Entity::Enemy>(Game::TEX_EnemyBossSquad, 500, std::min(5,boss_id), true);
                    entity->pos = Position(pos, 48 + 64);
                    entity->vel = Velocity(20, 100);
                    parent.common.sdl_inf->tim.add_timer(200, EnemySh(this_, entity));
                    return entity;
                });
            };
        };

        void S_Running::add_enemy(bool bullets) {
            with_new_entity([&](Entity::Name) {
                int16_t x     = parent.common.rng.i_dis_0_width  (parent.common.rng.random_eng);
                double  type  = parent.common.rng.d_dis_0_1      (parent.common.rng.random_eng);
                int bullet_rel = std::min(player.bullet_level * 2, 45);
                // we don't let enemies spawn too past
                x = std::min<int16_t>(x, parent.common.res.width - 100);
                // we reuse the random number generator of the bg_particles, TODO: rename it
                if (type > 0.9) {
                    auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(Game::TEX_Enemy3, 300, 2 + 0.5 * player.bullet_level));
                    entity->pos = Position(x, 2);
                    entity->vel = Velocity(0,95 + bullet_rel);
                    if (bullets) parent.common.sdl_inf->tim.add_timer(400, EnemySh(this_, entity, Game::TEX_EnemyLaser));
                    return entity;
                } else if (type > 0.4) {
                    auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(Game::TEX_Enemy2, 200, 1 + 0.35 * player.bullet_level));
                    entity->pos = Position(x, 2);
                    entity->vel = Velocity(0,85 + bullet_rel);
                    if (bullets) parent.common.sdl_inf->tim.add_timer(400, EnemySh(this_, entity, Game::TEX_EnemyLaser));
                    return entity;
                } else {
                    auto entity = std::shared_ptr<Entity::Enemy>(new Entity::Enemy(Game::TEX_Enemy1, 100, 1 + 0.25 * player.bullet_level));
                    entity->pos = Position(x, 5);
                    entity->vel = Velocity(0,70 + bullet_rel);
                    if (bullets) parent.common.sdl_inf->tim.add_timer(400, EnemySh(this_, entity, Game::TEX_EnemyLaser));
                    return entity;
                } 

            });
        };
        void S_Running::maybe_add_enemy(double prob, bool bullets) {
            double  type  = parent.common.rng.d_dis_0_1(parent.common.rng.random_eng);
            if (type > 1-prob) add_enemy(bullets);
        };
        void S_Running::with_new_entity(std::function<std::shared_ptr<Entity::Type>(Entity::Name)> fn) {
                Entity::Name e = last_entity;
                ++last_entity;
                ent_mp.insert(std::pair<Entity::Name, std::shared_ptr<Entity::Type>>(e, fn(e)));
        };
    };
    void event_handler(Conc::Chan<SDL_Event>&    sdl_events   ,
                       Conc::Chan<Events::Type>& game_events  ) {
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
                                case SDLK_BACKSPACE: game_events.push(Events::Type(Events::TextInput('\b')));
                                                 break;
                                case SDLK_RETURN: game_events.push(Events::Type(Events::TextInput('\n')));
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
                case SDL_TEXTINPUT:
                        game_events.push(Events::Type(Events::TextInput(ev.text.text)));
                        break;
                default: break;
            }
        };
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
    namespace Entity {

/*
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , Enemy&          );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , PlayerBullet&   ); NOP
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , Player&         ); NOP
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , EnemyBullet&    );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Player&         , PowerUp&        );
            // enemy vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&          , Enemy&          );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&      x    , PlayerBullet&   ); 
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&          , EnemyBullet&    ); NOP
            bool on_collision(std::shared_ptr<GameState::S_Running>   , Enemy&          , PowerUp&        ); NOP
            // player_bullet vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PlayerBullet&   , PlayerBullet&   ); NOP
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PlayerBullet&   , EnemyBullet&    );
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PlayerBullet&   , PowerUp&        );
            // enemy_bullet vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running>   , EnemyBullet&    , EnemyBullet&    ); NOP
            bool on_collision(std::shared_ptr<GameState::S_Running>   , EnemyBullet&    , PowerUp&        ); NOP
            // power up vs the world
            bool on_collision(std::shared_ptr<GameState::S_Running>   , PowerUp&        , PowerUp&        ); NOP
 */

        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, Player&, Player&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, Player&, PlayerBullet&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, Enemy&, Enemy&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, Enemy&, EnemyBullet&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, Enemy&, PowerUp&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, PlayerBullet&,  PlayerBullet&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, EnemyBullet&,  EnemyBullet&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, EnemyBullet&,  PowerUp&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, PowerUp&,  PowerUp&) {
            return false;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, PlayerBullet&,  PowerUp&) {
            return false;
        };

        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Player&, Enemy& e) {
            // TODO: balance tweaks, suggestions for this?
            e.kill();
            run->do_damage(e.health * 3);
            run->maybe_add_enemy(0.05);
            run->add_enemy();
            return true;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Player&, PowerUp& pup) {
            // TODO: powerups
            if (pup.kind == PowerUp::Shield) {
                run->parent.common.sdl_inf->play_sfx(Game::SFX_ShieldEnabled);
                run->add_shield(40);
            } else if (pup.kind == PowerUp::Bolt) {
                run->player.bullet_level++;
            }
            pup.kill();
            return true;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Player&, EnemyBullet& b) {
            run->do_damage(15);
            b.kill();
            return true;
        };

        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, PlayerBullet& e, EnemyBullet& b) {
            b.kill();
            e.kill();
            return true;
        };

        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Enemy& e, PlayerBullet& b) {
            e.health -= ceil(run->player.bullet_level);
            b.kill();
            if (e.health <= 0) {
                e.kill();
                if (run->ent_mp.size() < 50) {
                    run->add_enemy();
                }
                run->maybe_add_enemy(0.05);
                run->parent.maybe(0.05 * (1/(1+(run->player.bullet_level/6))), [&]() {
                    run->with_new_entity([&](Entity::Name) {
                        auto p = std::shared_ptr<Entity::PowerUp>(new Entity::PowerUp(Entity::PowerUp::Shield));
                        p->txt_name = Game::TEX_PowerupShield;
                        p->pos = e.pos;
                        p->vel = Velocity(0, 45);
                        return p;
                    });
                });
                run->parent.maybe(0.1 * (1/(1+(run->player.bullet_level/4))), [&]() {
                    run->with_new_entity([&](Entity::Name) {
                        auto p = std::shared_ptr<Entity::PowerUp>(new Entity::PowerUp(Entity::PowerUp::Bolt));
                        p->txt_name = Game::TEX_PowerupBolt;
                        p->pos = e.pos + Position(0,40);
                        p->vel = Velocity(0, 45);
                        return p;
                    });
                });
                run->add_points(e.score*1.5);
            };
            run->maybe_add_enemy(0.05 * 3/(1+run->player.bullet_level));
            run->add_points(e.score/4);
            return true;
        };
        Collision collision;
    };
};