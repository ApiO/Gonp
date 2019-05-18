#include <engine/pge.h>
#include <runtime/memory.h>
#include "game_types.h"
#include "systems/camera_system.h"
#include "systems/ball_system.h"
#include "systems/arena_system.h"
//#include "dev/fps_widget.h"

using namespace pge;

// consts and types
namespace app
{
  //static FpsWidget fps_widget;
  const glm::vec2 virtual_resolution(1280, 800);
  Game game;
}


// main fund definitions
namespace app
{

  void init()
  {
    // setups app res
    game.package = application::resource_package("packages/default");

    resource_package::load(game.package);
    resource_package::flush(game.package);

    physics::show_debug(game.show_physics);
    application::show_culling_debug(game.show_aabb);
    game.num_players = 2;

    // setups game world
    game.world = application::create_world();

    // setups aspect & viewport
    window::get_resolution(game.screen_width, game.screen_height);
    f32 target_aspect = virtual_resolution.x / virtual_resolution.y;
    {
      i32 width = game.screen_width;
      i32 height = (i32)(width / target_aspect + 0.5f);

      if (height > game.screen_height) {
        height = game.screen_height;
        width = (int)(height * target_aspect + 0.5f);
      }
      game.viewport = application::create_viewport((game.screen_width / 2) - (width / 2), (game.screen_height / 2) - (height / 2), width, height);
    }

    // setups HUD
    game.hud = application::create_world();
    game.camera_ortho = world::spawn_camera(game.hud, target_aspect, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
    camera::set_projection_type(game.hud, game.camera_ortho, PROJECTION_ORTHOGRAPHIC);
    camera::set_orthographic_projection(game.hud, game.camera_ortho, -game.screen_width * .5f, game.screen_width * .5f, -game.screen_height * .5f, game.screen_height * .5f);
    camera::set_vertical_fov(game.hud, game.camera_ortho, 45.f);
    camera::set_near_range(game.hud, game.camera_ortho, -1.f);
    camera::set_far_range(game.hud, game.camera_ortho, 1.f);

    Allocator &a = memory_globals::default_allocator();
    camera_system::init(virtual_resolution);
    ball_system::init(a);
    arena_system::init(a);

    //fps_widget.init(game.world, "fonts/consolas.24/consolas.24", (i32)virtual_resolution.x, (i32)virtual_resolution.y);
  }

  void update(f64 delta_time)
  {
    //fps_widget.update(delta_time);

    world::update(game.world, delta_time);
    world::update(game.hud, delta_time);

    if (keyboard::pressed(KEYBOARD_KEY_ESCAPE))
      application::quit();

    if (keyboard::pressed(KEYBOARD_KEY_P)) {
      game.show_physics = !game.show_physics;
      physics::show_debug(game.show_physics);
    }

    if (keyboard::pressed(KEYBOARD_KEY_B)) {
      game.show_aabb = !game.show_aabb;
      application::show_culling_debug(game.show_aabb);
    }

    arena_system::update(delta_time);
    ball_system::update(delta_time);
    camera_system::update(delta_time);
  }

  void render()
  {
    application::render_world(game.hud, game.camera_ortho, game.viewport);
    application::render_world(game.world, camera_system::get_id(), game.viewport);
  }

  void shutdown()
  {
    //fps_widget.shutdown();

    arena_system::shutdown();
    ball_system::shutdown();
    camera_system::shutdown();

    world::despawn_camera(game.hud, game.camera_ortho);
    application::destroy_world(game.hud);

    application::destroy_world(game.world);

    application::destroy_viewport(game.viewport);

    resource_package::unload(game.package);
    application::release_resource_package(game.package);
  }
}
