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
        struct HudRect {
            Position p1;
            Position p2;
            SDL_Color col;
            bool fill;
        };
        class Hud {
        public:
            int start_hud = 0;
            std::vector<HudText> txts;
            std::vector<HudItem> items;
            std::vector<HudRect> rects;
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
            void add_rect(HudRect r) {
                rects.push_back(r);
            };
        };

        class Eng { /* this is probably a good fit for inheritance
                       but adding indirections with pointers/reference is boring */
        private:
            std::shared_ptr<Game::sdl_info> sdl_inf;
            GameState::Type&   s;
            uint16_t           fps_relation;
            uint64_t counter;

        public:
            Eng(std::shared_ptr<Game::sdl_info>  sdl_inf     ,
                GameState::Type&                 s           ,
                uint16_t                         fps_relation,
                uint64_t counter = 0
               ) : sdl_inf(sdl_inf), s(s), fps_relation(fps_relation) , counter(counter) {};

            int64_t inline get_counter() { return counter; }

            Game::LSit render_HighScores(std::shared_ptr<GameState::S_HighScores>);
            Game::LSit render_Running   (std::shared_ptr<GameState::S_Running   >);
            Game::LSit render_QuitGame  (std::shared_ptr<GameState::S_QuitGame  >);
            Game::LSit render_Menu      (std::shared_ptr<GameState::S_Menu      >);
            Game::LSit render_Credits   (std::shared_ptr<GameState::S_Credits   >);

            void apply_HighScores(std::shared_ptr<GameState::S_HighScores>);
            void apply_Running   (std::shared_ptr<GameState::S_Running   >);
            void apply_QuitGame  (std::shared_ptr<GameState::S_QuitGame  >);
            void apply_Menu      (std::shared_ptr<GameState::S_Menu      >);
            void apply_Credits   (std::shared_ptr<GameState::S_Credits   >);

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

                return match(*(s.ms)
                            , [&](std::shared_ptr<GameState::S_HighScores> x) { return eng.render_HighScores(x); }
                            , [&](std::shared_ptr<GameState::S_Running   > x) { return eng.render_Running   (x); }
                            , [&](std::shared_ptr<GameState::S_QuitGame  > x) { return eng.render_QuitGame  (x); }
                            , [&](std::shared_ptr<GameState::S_Menu      > x) { return eng.render_Menu      (x); }
                            , [&](std::shared_ptr<GameState::S_Credits   > x) { return eng.render_Credits   (x); });
            });
        };

        void inline expensive_handler(Conc::VarL<GameState::Type>& svar) {
                uint64_t counter = 0;
                Game::Utils::tempo(10, [&](uint32_t&, auto debt) {
                    return svar.modify([&](GameState::Type& s) {
                        uint16_t fps_relation = 10 + debt.count();
                        Eng eng(s.common.sdl_inf, s, fps_relation, counter);
                        s.common.sdl_inf->tim.advance(fps_relation);
                        auto x = match(*(s.ms)
                                    , [&](std::shared_ptr<GameState::S_HighScores> x) { eng.apply_HighScores(x); return Game::KeepLooping; }
                                    , [&](std::shared_ptr<GameState::S_Running   > x) { eng.apply_Running   (x); return Game::KeepLooping; }
                                    , [&](std::shared_ptr<GameState::S_QuitGame  > x) { eng.apply_QuitGame  (x); return Game::BreakLoop  ; }
                                    , [&](std::shared_ptr<GameState::S_Menu      > x) { eng.apply_Menu      (x); return Game::KeepLooping; }
                                    , [&](std::shared_ptr<GameState::S_Credits   > x) { eng.apply_Credits   (x); return Game::KeepLooping; });
                        counter = eng.get_counter();
                        return x;
                    });
                });
        };
    };
};