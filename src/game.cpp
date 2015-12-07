#include "game.h"

namespace Game {
    sdl_info::sdl_info(const char* game_name, std::string font_name, int fps_param) {
            window = SDL_CreateWindow(game_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
            if (window == NULL) { throw SDLError(); }
            win_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
            if (fps_param > 0) fps = fps_param;
            fonts[Small]  = TTF_OpenFont(font_name.c_str(), 20);
            fonts[Normal] = TTF_OpenFont(font_name.c_str(), 24);
            fonts[Huge]   = TTF_OpenFont(font_name.c_str(), 32);
            gm_name = game_name;
    }
    sdl_info::~sdl_info() {
        for (const auto& pair : txts) {
            SDL_DestroyTexture(pair.second.texture);
        };
        SDL_DestroyWindow(window);
    }

    Resolution sdl_info::get_current_res() {
        SDL_DisplayMode current;
        SDL_GetCurrentDisplayMode(0, &current); // we assume nothing goes wrong..
        return Resolution(current.w, current.h);
    };

    void sdl_info::set_background(std::string bg_name) {
            music = Mix_LoadMUS(bg_name.c_str());
            Mix_PlayMusic(music, -1);
            Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
    };

    void sdl_info::load_png(std::string key, std::string path) {
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

    void sdl_info::load_sfx(std::string key, std::string path) {
        sfx[key] = Mix_LoadWAV(path.c_str());
    };

    void sdl_info::play_sfx(std::string key) {
        Mix_PlayChannel(-1, sfx[key], 0);
    };

    void sdl_info::render_text(int x, int y, FontID f_id, SDL_Color txt_color, std::string text) {
        SDL_Rect  pos;
        SDL_Surface* textSurface = TTF_RenderUTF8_Solid(fonts[f_id], text.c_str(), txt_color);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(win_renderer, textSurface);
        if (textSurface == NULL) return;
        pos.h = textSurface->h;
        pos.w = textSurface->w;
        pos.x = x;
        pos.y = y;
        SDL_FreeSurface(textSurface);
        SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(win_renderer, textTexture, NULL, &pos);
        SDL_DestroyTexture(textTexture);
    };
}