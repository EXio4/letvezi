#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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
            /*std::ostringstream cnvt;

            cnvt.str("SDL Error: ");

            cnvt << info; */

            return info.c_str();
        }
    };

    enum LSit {
        KeepLooping,
        BreakLoop,
    };

    class sdl_info {
        public:
            unsigned int fps = 60; /* fps >= 0 */
            SDL_Window*   window        = NULL;
            SDL_Renderer* win_renderer  = NULL;
            std::map<std::string, SDL_Texture*> map;
            /* std::string til I manage to stop being lazy */

            sdl_info(char* game_name, int width, int height, int fps_param=60) {
                window = SDL_CreateWindow(game_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
                if (window == NULL) { throw SDLError(); }
                win_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
                if (fps_param > 0) fps = fps_param;
            }
            ~sdl_info() {
                SDL_DestroyWindow(window);
                for (const auto &pair : map) {
                    SDL_DestroyTexture(pair.second);
                }
            }
            void load_png(std::string key, std::string path) {
                 {
                    auto it = map.find(key);
                    if (it != map.end()) return;
                 }
                 SDL_Surface* loadedSurface = IMG_Load(path.c_str());
                 if (loadedSurface == NULL) { throw SDLError(IMG_GetError()); }
                 SDL_Texture* texture = SDL_CreateTextureFromSurface(win_renderer, loadedSurface);
                 SDL_FreeSurface(loadedSurface);
                 map[key] = texture;
            }

            void with(std::string key, std::function<void(SDL_Texture*)> fn) {
                auto it = map.find(key);
                if (it == map.end()) { throw SDLError("[KEY] image not found"); }
                auto surf = it->second;

                return fn(surf);
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

                while(true) {
                    SDL_Event e;
                    while( SDL_PollEvent(&e) != 0) {
                        sdl_events.push(e);
                    }
                    auto t_start = std::chrono::steady_clock::now();
                    LSit x = render_handler(game_state, fps_relation);
                    switch(x) {
                        BreakLoop: return;
                                   break;
                        default:   break;
                    }
                    auto t_finish = std::chrono::steady_clock::now();
                    auto dif = std::chrono::duration_cast<std::chrono::milliseconds>(t_finish - t_start);
                    auto sleep_time = std::chrono::milliseconds(fps_relation) - dif;

                    SDL_RenderPresent(win_renderer);

                    if (sleep_time > std::chrono::milliseconds(0)) {
                        std::this_thread::sleep_for(sleep_time);
                    }
                }
            };
    };
}