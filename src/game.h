#pragma once

#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <map>
#include <thread>
#include <future>
#include <mutex>
#include <stdexcept>
#include <boost/optional.hpp>
#include "conc.h"
#include "timer.h"

namespace Game {

    /*
            load_png("bg_star"         , "art/img/bg_star.png"            );

            load_png("player"          , "art/img/player.png"             );
            load_png("player_laser"    , "art/img/player_laser.png"       );
            load_png("player_life"     , "art/img/player_life.png"        );
            load_png("player_shield"   , "art/img/player_shield.png"      );

            load_png("enemy_1"         , "art/img/enemy_1.png"            );
            load_png("enemy_2"         , "art/img/enemy_2.png"            );
            load_png("enemy_3"         , "art/img/enemy_3.png"            );
            load_png("enemy_boss"      , "art/img/enemy_boss.png"         );
            load_png("enemy_laser"     , "art/img/enemy_laser.png"        );
            load_png("enemy_boss_laser", "art/img/enemy_boss_laser.png"   );
            load_png("enemy_boss_squad", "art/img/enemy_boss_squad.png"   );

            load_png("powerup_shield"  , "art/img/powerup_shield.png"     );
            load_png("powerup_bolt"    , "art/img/powerup_bolt.png"       );

            load_sfx("player_laser"    , "art/sfx/player_laser.ogg"       );
            load_sfx("shield_enabled"  , "art/sfx/player_laser.ogg"       );
     */
    enum TextureID {
        TEX_BackgroundStar = 0x01,
        TEX_Player         = 0x10,
        TEX_PlayerLaser    = 0x11,
        TEX_PlayerLife     = 0x12,
        TEX_PlayerShield   = 0x13,
        TEX_Enemy1         = 0x21,
        TEX_Enemy2         = 0x22,
        TEX_Enemy3         = 0x23,
        TEX_EnemyBoss      = 0x25,
        TEX_EnemyBossSquad = 0x26,
        TEX_EnemyLaser     = 0x28,
        TEX_EnemyBossLaser = 0x29,
        TEX_PowerupShield  = 0x31,
        TEX_PowerupBolt    = 0x32,
    };
    enum SFX_ID {
        SFX_PlayerLaser   = 0x01,
        SFX_ShieldEnabled = 0x02
    };

    class SDLError: public std::runtime_error {
        private:
            std::string info;
        public:

        SDLError(std::string x)
            : std::runtime_error("SDL Error")
            { info = x; }
        SDLError(const char* x)
            : std::runtime_error("SDL Error")
            { info = x; }
        SDLError()
            : std::runtime_error("SDL Error")
            { info = SDL_GetError(); }

        virtual const char* what() const throw() {
            return info.c_str();
        }
    };

    enum LSit {
        KeepLooping,
        BreakLoop,
    };

    enum FontID {
        Small   ,
        Normal  ,
        Huge
    };

    struct Resolution {
        int16_t width;
        int16_t height;
        Resolution(int16_t width, int16_t height) : width(width), height(height) {}
    };
    struct TextureInfo {
        SDL_Texture* texture;
        int width;
        int height;
        TextureInfo(SDL_Texture* txt, int width, int height) : texture(txt), width(width), height(height) {}
        TextureInfo() : texture(NULL), width(0), height(0) {}
    };

    namespace Utils {
        template <typename FN>
        auto tempo(uint32_t expected_time_loop, FN&& fn) {
                    auto debt = std::chrono::milliseconds(0);
                    while(true) {

                        auto t_start = std::chrono::steady_clock::now();

                        Game::LSit x = fn(expected_time_loop, debt);
                        switch(x) {
                            case BreakLoop: return;
                                            break;
                            default:   break;
                        }

                        auto t_finish = std::chrono::steady_clock::now();
                        auto dif = std::chrono::duration_cast<std::chrono::milliseconds>(t_finish - t_start);
                        auto sleep_time = std::chrono::milliseconds(expected_time_loop) - dif;

                        debt = debt / 2;

                        if (sleep_time >= std::chrono::milliseconds(0)) {
                            std::this_thread::sleep_for(sleep_time + std::chrono::milliseconds(1));
                        } else {
                            /* if we are here, that means we have got a negative sleep time
                            * iow, we need to "advance" more in the next frame to make up for the slow frame we had previously
                            */
                            debt = -sleep_time;
                        }
                    };

        };
    };

    class sdl_info {
    private:
            uint16_t fps = 60; /* fps >= 0 */
            std::map<TextureID, TextureInfo>  txts;
            std::map<FontID,TTF_Font*> fonts;
            std::map<SFX_ID, Mix_Chunk*> sfx;
            std::string gm_name;
            Mix_Music* music;
        public:
            SDL_Window*   window        = NULL;
            SDL_Renderer* win_renderer  = NULL;

            Timer tim;

            sdl_info(const char* game_name, std::string font_name,int fps_param=60);
            sdl_info(const sdl_info&) = delete;
            sdl_info& operator=(const sdl_info&) = delete;
            ~sdl_info();

            Resolution get_current_res();

            void set_background(std::string bg_name);

            void load_png(TextureID key, std::string path);

            void load_sfx(SFX_ID key, std::string path);
            void play_sfx(SFX_ID key);

            template <typename FN>
            void loading_screen(FN&& fn) {
                using LoadCB = std::tuple<std::string,std::function<void(sdl_info&)>>;
                Resolution res = get_current_res();
                int start_y   = res.height/4;
                int finish_y  = 3 * start_y;
                int current_y = start_y;
                int accel = 32;
                std::queue<LoadCB> chan;
                fn(chan);
                while (1) {
                    SDL_SetRenderDrawColor(win_renderer, 0, 0, 0, 255);
                    SDL_RenderClear(win_renderer);
                    SDL_Color color = {255, 0, 255, 255};
                    render_text(res.width/4, res.height/4, Huge, color, gm_name);

                    render_text(res.width/4, res.height/2, Normal, color, "Loading game");
                    render_text(4*(res.width/5), current_y, Small, color, "~");
                    if (current_y > finish_y || current_y < start_y) accel *= -1;
                    current_y += accel;

                    if (chan.size() == 0) break;
                    LoadCB x = chan.front();
                    chan.pop();
                    render_text(res.width/3, 4*(res.height/5), Normal, color, std::get<0>(x));
                    SDL_RenderPresent(win_renderer);
                    {
                        auto t_start = std::chrono::steady_clock::now();
                        (std::get<1>(x))(*this);
                        auto t_finish = std::chrono::steady_clock::now();
                        auto diff = t_finish - t_start;
                        std::this_thread::sleep_for(std::chrono::milliseconds(16) - diff);
                    };
                };
                SDL_SetRenderDrawColor(win_renderer, 0, 0, 0, 255);
                SDL_RenderClear(win_renderer);
                SDL_RenderPresent(win_renderer);
                std::cout << "Game loaded" << std::endl;
            };

            void render_text(int x, int y, FontID f_id, SDL_Color txt_color, std::string text);

            template <typename F>
            auto with(TextureID key, F&& fn) {
                auto it = txts.find(key);
                if (it == txts.end()) { throw SDLError("with TextureID couldn't find texture, did you load all the resources?"); }
                auto surf = it->second;

                /* we use .at as if it didn't exist, we already threw a nice error message */
                return fn(surf);
            }

            const std::map<TextureID,TextureInfo>& textures() {
                return txts;
            }

            /* the second parameter of render_handled specifies the proportion of a second it should work with
             *   e.g. x being 1000 represents it has to "process" a whole second of time
             *   x is represented in ms
             */
            template <typename EV, typename S>
            void loop(std::function<void(Conc::Chan<SDL_Event>&,Conc::Chan<EV>&)> event_handler  ,
                      std::function<void(Conc::Chan<EV>&,Conc::VarL<S>&)>         gs_handler     ,
                      std::function<void(Conc::VarL<S>&)>                         expensive_fn   ,
                      std::function<LSit(Conc::VarL<S>&,uint16_t)>                render_handler ,
                      S start_state) {

                Conc::Chan<SDL_Event> sdl_events;
                Conc::Chan<EV>        game_events;
                Conc::VarL<S>         game_state(start_state);

                /* the basic idea is:
                 *       > the event_handler converts [or ignores] SDL_Events into "Game Events" (GEVs)
                 *       > GEVs are processed by the GS handler, which will update the game state
                 *       >> the core idea behind this, is that the game states will be minimal and
                 *          won't care about things like acceleration or similar
                 *       > the render handler, shouldn't loop, and is special in the sense that it
                 *         runs in the main thread, and should apply acceleration to entities/etc
                 */

                std::thread ev_th(event_handler, std::ref(sdl_events),  std::ref(game_events));
                std::thread gevs_th(gs_handler , std::ref(game_events), std::ref(game_state));
                std::thread exp_th(expensive_fn, std::ref(game_state)); 

                ev_th.detach();
                gevs_th.detach();
                exp_th.detach();
                uint16_t fps_relation = 1000/fps;

                Utils::tempo(fps_relation, [&](uint32_t&, auto debt) {
                    SDL_Event e;
                    while( SDL_PollEvent(&e) != 0) {
                        sdl_events.push(e);
                    }
                    LSit x = render_handler(game_state, fps_relation + debt.count() + 2);
                    SDL_RenderPresent(win_renderer);
                    return x;
                });
            };
    };
}