

#include <memory>
#include "game.h"

namespace game {

    std::shared_ptr<sdl_info> init(int width, int height) {
        return std::shared_ptr<sdl_info>(new sdl_info(width,height));
    }

}