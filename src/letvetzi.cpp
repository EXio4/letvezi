#include "letvetzi.h"
namespace Letvetzi {
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
                            run->shooting = ev.key_down;
                            {
                                s.common.sdl_inf->tim.add_timer(0, Ev(s));
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
                            auto speed = Velocity(look[std::min(6, run->bullet_level)-1], 0);
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
                                    run->add_life(-run->lives);
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
        class EvBoss {
          private:
             std::shared_ptr<S_Running> run;
             std::shared_ptr<Entity::Type> en;
          public:
            EvBoss(std::shared_ptr<S_Running> run, std::shared_ptr<Entity::Type> en) : run(run), en(en) {
            };
            void operator() () {
                if (!en->killed) {
                    run->with_new_entity([&](Entity::Name) {
                        auto b = std::shared_ptr<Entity::EnemyBullet>(new Entity::EnemyBullet());
                        b->txt_name = "enemy_laser";
                        b->pos = en->pos + Position(55,40);
                        b->vel = Velocity(0, 100);
                        return b;
                    });
                    run->parent.common.sdl_inf->tim.add_timer(1000, *this);
                };
            };
        };

        int boss_rate = 25000;
        void Type::Common::add_bg_particle(int16_t start_y) {
            int16_t x     = rng.i_dis_0_width (rng.random_eng);
            int16_t speed = rng.i_dis_40_100  (rng.random_eng);
            int16_t ysped = rng.i_dis_40_100  (rng.random_eng);
            bg_particles.push_front(Particle("bg_star", Position(Vec<PositionT>(x,start_y)), Velocity(ysped-70, speed/2)));
        };
        void Type::maybe(double prob, std::function<void()> fn) {
            double  p  = common.rng.d_dis_0_1 (common.rng.random_eng);
            if (p > 1-prob) fn();
        };

        S_Running::S_Running(Type& parent) : parent(parent) {
            ent_mp[Entity::PlayerID()] = std::shared_ptr<Entity::Type>(new Entity::Player(parent.common.cnt.player_original, Velocity(0,0)));
            for (int i=0; i<3; i++) {
                add_enemy();
            };
            maybe_add_enemy(0.75);
            maybe_add_enemy(0.5);
            maybe_add_enemy(0.25);
            maybe_add_enemy(0.1);
        };
        S_Credits::S_Credits(Type& parent, MState prev) : parent(parent),
                    text_pos(Position(parent.common.res.width/4, parent.common.res.height + 32)),
                    previous(prev) {};

        void S_Running::add_bullet(Velocity vel) {
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
            parent.common.sdl_inf->play_sfx("player_laser");
        }
        void S_Running::add_points(uint32_t p) {
            points += p;
            til_boss -= p;
            if (til_boss <= 0) {
                start_boss(1 + bosses_killed);
                til_boss = (1 + bosses_killed) * boss_rate;
            };
        };
        void S_Running::add_life(unsigned int li) {
            lives += li;
            if (lives == 0) {
                parent.common.persistent->high_scores.add_score(parent.common.persistent->user_data.player_name, points);
                parent.high_scores(points);
            };
        };
        void S_Running::start_boss(int boss_id) {
            for (int i=0; i<1+boss_id; i++) {
                double  p  = parent.common.rng.d_dis_0_1 (parent.common.rng.random_eng);
                int    pos = parent.common.res.width / 4 + p * 2 * parent.common.res.width/4;
                with_new_entity([&](Entity::Name) {
                    auto entity = std::make_shared<Entity::Enemy>(500, 3+2*std::min(5,bullet_level)*boss_id, true);
                    entity->pos = Position(pos, 48);
                    entity->vel = Velocity(40,0);
                    entity->txt_name = "enemy_boss";
                    parent.common.sdl_inf->tim.add_timer(400, EvBoss(this_, entity));
                    return entity;
                });
            };
            for (int i=0; i<1+(boss_id * 2); i++) {
                double  p  = parent.common.rng.d_dis_0_1 (parent.common.rng.random_eng);
                int pos = parent.common.res.width / 4 + p * 2 * parent.common.res.width/4;
                with_new_entity([&](Entity::Name) {
                    auto entity = std::make_shared<Entity::Enemy>(500, std::min(5,boss_id), true);
                    entity->pos = Position(pos, 48 + 64);
                    entity->vel = Velocity(20, 100);
                    entity->txt_name = "enemy_boss_squad";
                    parent.common.sdl_inf->tim.add_timer(400, EvBoss(this_, entity));
                    return entity;
                });
            };
        };

        void S_Running::add_enemy() {
            with_new_entity([&](Entity::Name) {
                int16_t x     = parent.common.rng.i_dis_0_width  (parent.common.rng.random_eng);
                double  type  = parent.common.rng.d_dis_0_1      (parent.common.rng.random_eng);
                int bullet_rel = std::min(bullet_level * 2, 45);
                // we don't let enemies spawn too past
                x = std::min<int16_t>(x, parent.common.res.width - 100);
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
        void S_Running::maybe_add_enemy(double prob) {
            double  type  = parent.common.rng.d_dis_0_1(parent.common.rng.random_eng);
            if (type > 1-prob) add_enemy();
        };
        void S_Running::with_new_entity(std::function<std::shared_ptr<Entity::Type>(Entity::Name)> fn) {
                Entity::Name e = last_entity;
                ++last_entity;
                ent_mp[e] = fn(e);
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
        void Player::extra_render(std::shared_ptr<GameState::S_Running> gs, std::shared_ptr<Game::sdl_info> sdl_inf, int fps_rel) {
            if (shield > 0) {
                Render::Eng eng(sdl_inf, gs->parent, fps_rel);
                int alpha = 255;
                if (shield < 1000) alpha = shield/4;
                eng.render_pic(pos - Position(10,16), "player_shield", alpha);
                shield -= fps_rel;
                shield = std::max(0, shield);
            };
        };
        void PlayerBullet::extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int) {
        };
        void EnemyBullet::extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int) {
        };
        void PowerUp::extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info> , int) {
        };
        void Enemy::extra_render(std::shared_ptr<GameState::S_Running>, std::shared_ptr<Game::sdl_info>, int) {
        };


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

        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Player& p, Enemy& e) {
            // TODO: balance tweaks, suggestions for this?
            e.kill();
            if (p.shield <= 5) {
                run->add_life(-1);
                run->maybe_add_enemy(0.05);
            };
            run->add_enemy();
            return true;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Player& pl, PowerUp& pup) {
            // TODO: powerups
            if (pup.kind == PowerUp::Shield) {
                run->parent.common.sdl_inf->play_sfx("shield_enabled");
                pl.shield += std::max(1500, 3000 - pl.shield/8);
            } else if (pup.kind == PowerUp::Bolt) {
                run->bullet_level++;
            }
            pup.kill();
            return true;
        };
        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Player& p, EnemyBullet& b) {
            if (!p.shield) {
                run->add_life(-1);
            };
            b.kill();
            return true;
        };

        bool Collision::on_collision(std::shared_ptr<GameState::S_Running>, PlayerBullet& e, EnemyBullet& b) {
            b.kill();
            e.kill();
            return true;
        };

        bool Collision::on_collision(std::shared_ptr<GameState::S_Running> run, Enemy& e, PlayerBullet& b) {
            e.health -= ceil(run->bullet_level);
            b.kill();
            if (e.health <= 0) {
                e.kill();
                if (run->ent_mp.size() < 50) {
                    run->add_enemy();
                }
                run->maybe_add_enemy(0.05);
                run->parent.maybe(0.05 * (1/(1+(run->bullet_level/6))), [&]() {
                    run->with_new_entity([&](Entity::Name) {
                        auto p = std::shared_ptr<Entity::PowerUp>(new Entity::PowerUp(Entity::PowerUp::Shield));
                        p->txt_name = "powerup_shield";
                        p->pos = e.pos;
                        p->vel = Velocity(0, 45);
                        return p;
                    });
                });
                run->parent.maybe(0.1 * (1/(1+(run->bullet_level/4))), [&]() {
                    run->with_new_entity([&](Entity::Name) {
                        auto p = std::shared_ptr<Entity::PowerUp>(new Entity::PowerUp(Entity::PowerUp::Bolt));
                        p->txt_name = "powerup_bolt";
                        p->pos = e.pos + Position(0,40);
                        p->vel = Velocity(0, 45);
                        return p;
                    });
                });
                run->add_points(e.score*1.5);
            };
            run->maybe_add_enemy(0.05 * 3/(1+run->bullet_level));
            run->add_points(e.score/4);
            return true;
        };

        void PlayerBullet::out_of_screen(std::shared_ptr<GameState::S_Running>) {
            this->kill();
        };
        void EnemyBullet::out_of_screen(std::shared_ptr<GameState::S_Running>) {
            this->kill();
        };
        void PowerUp::out_of_screen(std::shared_ptr<GameState::S_Running>) {
            this->kill();
        };
        void Enemy::out_of_screen(std::shared_ptr<GameState::S_Running> run) {
            if (boss) {
                vel = -1 * vel;
            } else {
                this->kill();
                run->add_enemy();
            }
        };


        Collision collision;
    };
    namespace Render {
        bool collide(const SDL_Rect *r1, const SDL_Rect *r2) {
            return (SDL_HasIntersection(r1,r2) == SDL_TRUE);
        };

        Game::LSit Eng::render_QuitGame(std::shared_ptr<GameState::S_QuitGame>) {
            return Game::BreakLoop;
        };
        Game::LSit Eng::render_Running(std::shared_ptr<GameState::S_Running> run) {

            std::shared_ptr<Entity::Type> player_pointer = run->ent_mp[Entity::PlayerID()];

            /* draw entities on the world */
            for (auto curr = run->ent_mp.begin() ; curr != run->ent_mp.end() ;) {
                if (curr->second->killed) {
                    curr = run->ent_mp.erase(curr);
                } else {
                    curr++;
                };
            };

            for (auto& curr : run->ent_mp) {
                apply_velocity(curr.second->pos, curr.second->vel, s.common.res, fps_relation);
                render_pic(curr.second->pos, curr.second->txt_name);
                curr.second->extra_render(run, sdl_inf, fps_relation);
                if ((curr.second->pos.x > s.common.res.width  || curr.second->pos.x < 0) ||
                    (curr.second->pos.y > s.common.res.height || curr.second->pos.y < 0) ) {
                        curr.second->out_of_screen(run);
                };
            }

            // collision detection
            for (auto& curr : run->ent_mp) {
                if (curr.second->killed) continue;
                sdl_inf->with(curr.second->txt_name, [&](Game::TextureInfo text) {
                    SDL_Rect curr_rect { curr.second->pos.x, curr.second->pos.y , text.width, text.height };
                    for (auto& other : run->ent_mp) {
                        if (curr.second->killed)       break;    // ^ and this are used for avoiding extra `duplication`
                                                                 //   x when more than entity collide with the same enemy or so
                        if (other.first <= curr.first) continue; // we ignore ourselves (and previous checks)
                        if (other.second->killed)      continue; // we ignore dead entities
                        sdl_inf->with(other.second->txt_name, [&](Game::TextureInfo text2) {
                            SDL_Rect other_rect { other.second->pos.x, other.second->pos.y , text2.width , text2.height };
                            if (collide(&curr_rect, &other_rect)) {
                                Entity::collision(run, curr.second, other.second);
                            };
                        });
                    };
                });
            };

            {
                int32_t shield = dynamic_cast<Entity::Player*>(run->ent_mp[Entity::PlayerID()].get())->shield;
                Hud hud;
                SDL_Color txt_color = {200, 200, 200, 255}; // hud color
                hud.start_hud = s.common.res.height - 64;
                hud.add_text(s.common.res.width - 256 - 32 , s.common.res.height - 48, txt_color, "Points:  " + std::to_string(run->points));
                hud.add_text(s.common.res.width - 512 - 32 , s.common.res.height - 48, txt_color, "Shield: " + std::to_string(shield));
                hud.add_text(48, s.common.res.height - 48, txt_color, "Lives: ");
                Position pos(48 + 20 * 7, s.common.res.height - 48);
                for (int32_t i=0; i < run->lives; i++) {
                    hud.add_image("player_life", pos);
                    pos.x += 40;
                };

                render_hud(hud);
            };

            return Game::KeepLooping;
        };

        Game::LSit Eng::render_HighScores(std::shared_ptr<GameState::S_HighScores> hs) {
            {
                Hud hud;
                hud.start_hud = s.common.res.height - 64;
                {
                    SDL_Color player_name {200,   0, 200, 255};
                    SDL_Color points      {200, 200, 200, 255};
                    hud.add_text(Position(48, 64+16), points, "High scores");
                    Position pos(64, 64+48);
                    for (auto& r : s.common.persistent->high_scores.table()) {
                        hud.add_text(pos, player_name , r.name);
                        hud.add_text(pos + Position(s.common.res.width/3, 0) , points, std::to_string(r.points));
                        pos += Position(0,48);
                    };

                };
                SDL_Color txt_color {200, 200, 200, 255}; // hud color
                if (hs->points) {
                    hud.add_text(s.common.res.width - 256 - 32 , s.common.res.height - 48, txt_color, "Points:  " + std::to_string(*(hs->points)));
                } else {
                    hud.add_text(s.common.res.width - 256 - 64 , s.common.res.height - 48, txt_color, "Name: " + s.common.persistent->user_data.player_name);
                };
                SDL_Color game_over_c {255, 0, 0, 255};
                hud.add_text(48, s.common.res.height - 100, game_over_c, "GAME OVER");
                SDL_Color game_over_c2 = {0,0,255,255};
                hud.add_text(s.common.res.width/3, s.common.res.height - 48, game_over_c2, "Press [Return] to go back to the main menu");
                render_hud(hud);
            };
            return Game::KeepLooping;
        };

        Game::LSit Eng::render_Credits(std::shared_ptr<GameState::S_Credits> credits) {
            Position pos = credits->text_pos;
            Hud      hud;
            SDL_Color txt_color = {200, 200, 200, 255};
            if (pos.y < -200) *s.ms = credits->previous;
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
                if (pos.x >= s.common.res.width || pos.y >= s.common.res.height) break;
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
            apply_velocity(credits->text_pos, Velocity(0, -30), s.common.res, fps_relation);
            return Game::KeepLooping;
        };

        Game::LSit Eng::render_Menu(std::shared_ptr<GameState::S_Menu> s_menu) {
            {
                Hud hud;
                SDL_Color txt_color = {200, 200, 200, 255}; // hud color
                hud.start_hud = s.common.res.height - (256+128);;
                hud.add_text(Position(48, hud.start_hud + 32), txt_color, s_menu->menu.title, Game::Huge);
                SDL_Color pressed_option {255, 0,   0, 255};
                SDL_Color current_option {255, 0, 255, 255};
                SDL_Color other_option   {0,   0, 255, 255};
                // TODO: see how we could deal with the menu w/o using a normal loop
                // and still have an easy way to select the next/prev menu entry easily
                Position pos(128, hud.start_hud + 128); // start pos
                for (uint16_t i=0; i<s_menu->menu.opts.size(); i++) {
                    auto c = s_menu->menu.opts[i];
                    if (i == s_menu->menu.current) {
                        if (s_menu->menu.pressed > 0) {
                            hud.add_text(pos, pressed_option, c.text);
                            s_menu->menu.pressed -= fps_relation;
                            if (s_menu->menu.pressed <= 0) {
                                s_menu->menu.pressed = 0;
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
                hud.add_text(Position(s.common.res.width - (512+256) , hud.start_hud + 256     ), pressed_option, "Left/Right keys = Move between options");
                hud.add_text(Position(s.common.res.width - (512+256) , hud.start_hud + 256 + 32), pressed_option, "Space key       = Select current option");
                render_hud(hud);
            };
            return Game::KeepLooping;
        };

        void Eng::render_pic(Position pos, std::string txt_name, uint8_t alpha) {
            sdl_inf->with(txt_name, [&](Game::TextureInfo text) {
                SDL_Rect r;
                r.x = pos.x;
                r.y = pos.y;
                r.w = text.width;
                r.h = text.height;
                SDL_SetTextureAlphaMod(text.texture, alpha);
                SDL_RenderCopy(sdl_inf->win_renderer, text.texture, NULL, &r);
            });
        };

        void Eng::render_hud(const Hud& hud) {
                // hud
                SDL_SetRenderDrawColor(sdl_inf->win_renderer, 0, 0, 0, 255);
                {
                    SDL_Rect rect {0, hud.start_hud, s.common.res.width, s.common.res.height - hud.start_hud};
                    SDL_RenderFillRect(sdl_inf->win_renderer, &rect);
                };
                for (auto& txt : hud.txts) {
                    sdl_inf->render_text(txt.pos.x, txt.pos.y, txt.f_id, txt.col, txt.text);
                };
                for (auto& spr : hud.items) {
                    render_pic(Position(spr.pos.x, spr.pos.y), spr.texture);
                };
        };

        void Eng::render_background() {
                // apply background
                SDL_SetRenderDrawColor(sdl_inf->win_renderer, 75, 0, 60, 255);
                SDL_RenderClear(sdl_inf->win_renderer);

                sdl_inf->with("bg_star", [&](Game::TextureInfo bg_star) {
                    for(auto p = s.common.bg_particles.begin(); p != s.common.bg_particles.end() ;) {
                        apply_velocity(p->pos, p->vel, s.common.res, fps_relation);
                        SDL_Rect r {p->pos.x, p->pos.y, bg_star.width, bg_star.height};
                        SDL_RenderCopyEx(sdl_inf->win_renderer, bg_star.texture, NULL, &r, p->angle, NULL, SDL_FLIP_NONE);
                        p->angle += fps_relation/2;
                        if (p->pos.y >= s.common.res.height || p->pos.x <= 0 || p->pos.x >= s.common.res.width) {
                            p = s.common.bg_particles.erase(p);
                            s.common.add_bg_particle();
                        } else {
                            p++;
                        }
                    }
                });

                /**/
        }
    };
};