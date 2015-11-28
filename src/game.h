#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <map>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <conc.h>

namespace Game {

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

    class sdl_info {
    private:
            uint16_t fps = 60; /* fps >= 0 */
            std::map<std::string, TextureInfo>  txts;
            /* std::string til I manage to stop being lazy */
        public:
            SDL_Window*   window        = NULL;
            SDL_Renderer* win_renderer  = NULL;
            std::map<FontID,TTF_Font*> fonts;

            sdl_info(const char* game_name, std::string font_name, int fps_param=60) {
                window = SDL_CreateWindow(game_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
                if (window == NULL) { throw SDLError(); }
                win_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
                if (fps_param > 0) fps = fps_param;
                fonts[Small]  = TTF_OpenFont(font_name.c_str(), 18);
                fonts[Normal] = TTF_OpenFont(font_name.c_str(), 24);
                fonts[Huge]   = TTF_OpenFont(font_name.c_str(), 48);
            }
            ~sdl_info() {
                for (const auto &pair : txts) {
                    SDL_DestroyTexture(pair.second.texture);
                }
                SDL_DestroyWindow(window);
            }

            Resolution get_current_res() {
                SDL_DisplayMode current;
                SDL_GetCurrentDisplayMode(0, &current); // we assume nothing goes wrong..
                return Resolution(current.w, current.h);
            }

            void load_png(std::string key, std::string path) {
                 {
                    auto it = txts.find(key);
                    if (it != txts.end()) return;
                 }
                 SDL_Surface* loadedSurface = IMG_Load(path.c_str());
                 if (loadedSurface == NULL) { throw SDLError(IMG_GetError()); }
                 SDL_Texture* texture = SDL_CreateTextureFromSurface(win_renderer, loadedSurface);
                 SDL_FreeSurface(loadedSurface);
                 TextureInfo &inf = txts[key];
                 SDL_QueryTexture(texture, NULL, NULL, &(inf.width), &(inf.height));
                 inf.texture = texture;
            }

            void render_text(int x, int y, FontID f_id, SDL_Color txt_color, std::string text) {
                SDL_Rect  pos;
                SDL_Surface* textSurface = TTF_RenderUTF8_Solid(fonts[f_id], text.c_str(), txt_color);
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(win_renderer, textSurface);
                pos.h = textSurface->h;
                pos.w = textSurface->w;
                pos.x = x;
                pos.y = y;
                SDL_FreeSurface(textSurface);
                SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);
                SDL_RenderCopy(win_renderer, textTexture, NULL, &pos);
                SDL_DestroyTexture(textTexture);
            };

            template <typename F>
            auto with(std::string key, F&& fn) {
                auto it = txts.find(key);
                if (it == txts.end()) { throw SDLError("[KEY] image not found: " + key); }
                auto surf = it->second;

                /* we use .at as if it didn't exist, we already threw a nice error message */
                return fn(surf);
            }
            const std::map<std::string,TextureInfo>& textures() {
                return txts;
            }

            /* the second parameter of render_handled specifies the proportion of a second it should work with
             *   e.g. x being 1000 represents it has to "process" a whole second of time
             *   x is represented in ms
             */
            template <typename EV, typename S>
            void loop(std::function<void(Conc::Chan<SDL_Event>&,Conc::Chan<EV>&)> event_handler  ,
                      std::function<void(Conc::Chan<EV>&,Conc::VarL<S>&)>         gs_handler     ,
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

                ev_th.detach();
                gevs_th.detach();
                uint16_t fps_relation = 1000/fps;

                auto debt = std::chrono::milliseconds(0);
                while(true) {
                    SDL_Event e;
                    while( SDL_PollEvent(&e) != 0) {
                        sdl_events.push(e);
                    }
                    auto t_start = std::chrono::steady_clock::now();
                    LSit x = render_handler(game_state, fps_relation + debt.count());
                    switch(x) {
                        case BreakLoop: return;
                                        break;
                        default:   break;
                    }
                    auto t_finish = std::chrono::steady_clock::now();
                    auto dif = std::chrono::duration_cast<std::chrono::milliseconds>(t_finish - t_start);
                    auto sleep_time = std::chrono::milliseconds(fps_relation) - dif;

                    SDL_RenderPresent(win_renderer);

                    debt = debt / 2;

                    if (sleep_time >= std::chrono::milliseconds(0)) {
                        std::this_thread::sleep_for(sleep_time);
                    } else {
                        /* if we are here, that means we have got a negative sleep time
                         * iow, we need to "advance" more in the next frame to make up for the slow frame we had previously
                         */
                        debt = -sleep_time;
                    }
                };

            };
    };
}