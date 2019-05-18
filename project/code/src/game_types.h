#pragma once

#include "types.h"

namespace app
{
  using namespace pge;

  struct Game
  {
    Game();
    u64    world;
    u64    hud;
    u64    camera_ortho;
    u64    viewport;
    i32    screen_width;
    i32    screen_height;
    u64    package;
    u32    num_players;
    bool   show_physics;
    bool   show_aabb;
  };

  inline Game::Game() : show_physics(false), show_aabb(false) {}

  extern Game game;
}