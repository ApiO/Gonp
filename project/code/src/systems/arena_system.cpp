#include <runtime/memory.h>
#include <runtime/array.h>
#include <game_types.h>

#include <units/player.h>
#include "ball_system.h"
#include "arena_system.h"

// types & consts
namespace app
{
#define NUM_WALL NUM_PLAYER_MAX*2

  struct ArenaSystem
  {
    Player players[NUM_PLAYER_MAX];
    u64    walls[NUM_WALL];
  };

  static ArenaSystem system;

  struct Slot
  {
    Slot(glm::vec2 t, f32 r);
    glm::vec2 translation;
    f32       rotation;
  };
  inline Slot::Slot(glm::vec2 t, f32 r) : translation(t), rotation(r){}

  const Slot CONFIG_1[] = {
    Slot(glm::vec2(-580, -30), 0),
    Slot(glm::vec2(580, -30), 0)
  };

  const Slot CONFIG_2[] = {
    Slot(glm::vec2(580, -30), 0),
    Slot(glm::vec2(580, -30), 0)
  };

  const Slot *CONFIGS[] = {
    CONFIG_1
  };
}

// header definitions
namespace app
{
  namespace arena_system
  {
    void init(Allocator &a)
    {
      f32 pad = -30;
      glm::vec3 position;
      glm::vec3 scale(350, 1, 1);;

      const Slot *config = CONFIGS[game.num_players-2];

      for (u32 i = 0; i < game.num_players; i++){
        const Slot &slot = config[i];

        // TODO: envoyer une pose directement
        // inits player
        position = glm::vec3(slot.translation,0);
        system.players[i].init(position, i, a);

        // spawn top & bottom walls
        position = glm::vec3(position.x*.5f, 350 + pad, 0);

        system.walls[i*2] = world::spawn_unit(game.world, "units/wall/wall", position, IDENTITY_ROTATION, scale);

        position.y -= ARENA_HEIGHT;
        glm::quat rotation(glm::radians(glm::vec3(0, 0, 180)));

        system.walls[i*2+1] = world::spawn_unit(game.world, "units/wall/wall", position, rotation, scale);
      }
    }

    void shutdown(void)
    {
      for (u32 i = 0; i < game.num_players; i++){
        system.players[i].destroy();
        world::despawn_unit(game.world, system.walls[i]);
      }
      for (u32 i = game.num_players; i < game.num_players * 2; i++)
        world::despawn_unit(game.world, system.walls[i]);
    }

    void update(f64 delta_time)
    {
      for (u32 i = 0; i < game.num_players; i++)
        system.players[i].update(delta_time);
    }
    
    void contract_callback(u32 player)
    {
      for (u32 i = 0; i < game.num_players; i++){
        if (i == player) continue;
        system.players[i].contract();
      }
    }

    void goal_recieves_balls(const Array<ContactPoint> &contacts, const void *user_data)
    {
      u32 player_index = *(u32*)user_data;

      for (u32 i = 0; i < array::size(contacts); i++)
        ball_system::despawn(actor::unit(game.world, contacts[i].actor));

      for (u32 i = 0; i < game.num_players; i++)
        if (i == player_index){
          system.players[i].add_bonus();
        }
        else{
          system.players[i].add_points(array::size(contacts));
        }
    }
  }
}