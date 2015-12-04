#include "letvetzi.h"


namespace Letvetzi {
    void apply_velocity(Position& pos, Velocity vel, Game::Resolution res, int16_t fps_relation) {
        int16_t p_x = pos.x + (vel.x * res.width  * fps_relation) /100/1000;
        int16_t p_y = pos.y + (vel.y * res.height * fps_relation) /100/1000;
        pos.x = std::max<int16_t>(-64, std::min<int16_t>(p_x, res.width+64));
        pos.y = std::max<int16_t>(-64, std::min<int16_t>(p_y, res.height+64));
    };

    namespace GameState {
        int boss_rate = 25000;
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
    };

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
                pl.shield += std::max(1500, 3000 - pl.shield/8);
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
        Collision collision;
        Name PlayerID() {
            return Entity::Name(0);
        };
    }

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

    namespace Render {
        bool collide(const SDL_Rect *r1, const SDL_Rect *r2) {
            return (SDL_HasIntersection(r1,r2) == SDL_TRUE);
        };

        Game::LSit Eng::render_QuitGame() {
            return Game::BreakLoop;
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
                int32_t shield = dynamic_cast<Entity::Player*>(s.ent_mp[Entity::PlayerID()].get())->shield;
                Hud hud;
                SDL_Color txt_color = {200, 200, 200, 255}; // hud color
                hud.start_hud = s.res.height - 64;
                hud.add_text(s.res.width - 256 - 32 , s.res.height - 48, txt_color, "Points:  " + std::to_string(s.points));
                hud.add_text(s.res.width - 512 - 32 , s.res.height - 48, txt_color, "Shield: " + std::to_string(shield));
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

        Game::LSit Eng::render_HighScores() {
            {
                Hud hud;
                hud.start_hud = s.res.height - 64;
                {
                    SDL_Color player_name {200,   0, 200, 255};
                    SDL_Color points      {200, 200, 200, 255};
                    hud.add_text(Position(48, 64+16), points, "High scores");
                    Position pos(64, 64+48);
                    for (auto& r : s.persistent_data->high_scores.table()) {
                        hud.add_text(pos, player_name , r.name);
                        hud.add_text(pos + Position(s.res.width/3, 0) , points, std::to_string(r.points));
                        pos += Position(0,48);
                    };

                };
                SDL_Color txt_color {200, 200, 200, 255}; // hud color
                if (s.points > 0) {
                    hud.add_text(s.res.width - 256 - 32 , s.res.height - 48, txt_color, "Points:  " + std::to_string(s.points));
                } else {
                    hud.add_text(s.res.width - 256 - 64 , s.res.height - 48, txt_color, "Name: " + s.persistent_data->user_data.player_name);
                };
                SDL_Color game_over_c {255, 0, 0, 255};
                hud.add_text(48, s.res.height - 100, game_over_c, "GAME OVER");
                SDL_Color game_over_c2 = {0,0,255,255};
                hud.add_text(s.res.width/3, s.res.height - 48, game_over_c2, "Press [Return] to go back to the main menu");
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
                    case GameState::Type::HighScores:
                        return eng.render_HighScores();
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
                shield = std::max(0, shield);
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
};