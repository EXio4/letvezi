#pragma once
#include "letvezi.h"

namespace Letvezi {

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
            std::shared_ptr<Game::sdl_info> sdl_inf;
            GameState::Type&   s;
            uint16_t           fps_relation;

        public:
            Eng(std::shared_ptr<Game::sdl_info>  sdl_inf     ,
                GameState::Type&                 s           ,
                uint16_t                         fps_relation)
                : sdl_inf(sdl_inf), s(s), fps_relation(fps_relation) {};

            Game::LSit render_HighScores(std::shared_ptr<GameState::S_HighScores>);
            Game::LSit render_Running   (std::shared_ptr<GameState::S_Running   >);
            Game::LSit render_QuitGame  (std::shared_ptr<GameState::S_QuitGame  >);
            Game::LSit render_Menu      (std::shared_ptr<GameState::S_Menu      >);
            Game::LSit render_Credits   (std::shared_ptr<GameState::S_Credits   >);

            void render_pic(Position, std::string,uint8_t alpha=255);
            void render_background();
            void render_hud(const Hud&);
        };

        Game::LSit inline handler_game(std::shared_ptr<Game::sdl_info> gs,
                                Conc::VarL<GameState::Type>&    svar,
                                uint16_t                        fps_relation) {
            return svar.modify([&](GameState::Type& s) {
                Eng eng(gs, s, fps_relation);
                eng.render_background();
                gs->tim.advance(fps_relation);
                return match(*(s.ms)
                            , [&](std::shared_ptr<GameState::S_HighScores> x) { return eng.render_HighScores(x); }
                            , [&](std::shared_ptr<GameState::S_Running   > x) { return eng.render_Running   (x); }
                            , [&](std::shared_ptr<GameState::S_QuitGame  > x) { return eng.render_QuitGame  (x); }
                            , [&](std::shared_ptr<GameState::S_Menu      > x) { return eng.render_Menu      (x); }
                            , [&](std::shared_ptr<GameState::S_Credits   > x) { return eng.render_Credits   (x); });
            });
        };
    };
};