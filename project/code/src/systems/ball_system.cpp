#include "ball_system.h"

#include <engine/pge.h>
#include <runtime/memory.h>
#include <runtime/hash.h>

#include <game_types.h>

// internals types & consts
namespace app
{
  const u32 NUM_BALL_SPLIT_MAX        = 20;
  const f32 BALL_BONUS_ACC_FACTOR     = 1.4f;
  const f32 BALL_ACC_TICK             = 1.f;
  const f32 BALL_ACC_INCR             = 1.08f;
  const f32 BALL_DEFAULT_SPEED        = 12.f;
  const f32 BALL_SPAWN_ANGLE_VARIANCE = .15f;
  const u32 BALL_MAX_ACC              = 10;
  const f32 COUNTDOWN_DURATION       = 4.f;
  const f32 COUNTDOWN_SCALE_START    = 1.f;
  const f32 COUNTDOWN_SCALE_END      = 4.f;
  const f32 COUNTDOWN_SCALE_COEF     = (COUNTDOWN_SCALE_END - COUNTDOWN_SCALE_START) / COUNTDOWN_SCALE_END;


  struct Ball
  {
    u64 actor;
    u32 acc;
    f64 duration;
  };

  struct BallSystem
  {
    Hash<Ball> *balls;
    u64 countdown_text;
    f64 countdown_duration;
    u32 countdown_value;

    u64 countdown;
  };
  
  static BallSystem system;
  
  /*
  static void on_ball_contact(const Array<ContactPoint> &contacts, const void *user_data)
  {
    printf("%u\n", *(u64*)user_data);
  }
  */

  static void spawn_ball(const glm::vec3 &p, const glm::vec3 &v, f64 duration = 0, u32 acc = 0)
  {
    u64 id = world::spawn_unit(game.world, "units/ball/ball", p, IDENTITY_ROTATION);
    
    Ball ball;
    ball.duration = duration;
    ball.acc      = acc;

    hash::set(*system.balls, id, ball);

    Ball &b = *hash::get(*system.balls, id);
    
    // setups physics
    b.actor = unit::actor(game.world, id, 0);
    actor::set_velocity(game.world, b.actor, v);
    //actor::set_touched_callback(game.world, b.actor, on_ball_contact, &b.actor);
  }
}

// definitions
namespace app
{

  namespace ball_system
  {
    void init(Allocator &a)
    {
      system.balls = MAKE_NEW(a, Hash<Ball>, a);
      hash::reserve(*system.balls, NUM_BALL_SPLIT_MAX * 2);

      system.countdown_text     = world::spawn_text(game.hud, "fonts/kristen.itc.36/kristen.itc.36", NULL, TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION, 4.f, Color(255, 255, 255, 255));
      system.countdown_duration = 0;

      //system.countdown = world::spawn_unit(game.world, "units/countdown/countdown", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      //unit::play_animation(game.world, system.countdown, "countdown", 0, 0, true);
    }

    void shutdown(void)
    {
      MAKE_DELETE((*system.balls->_data._allocator), Hash<Ball>, system.balls);
    }

    void update(f64 delta_time)
    {      
      //return;

      // handles spawn & countdown
      if (!hash::size(*system.balls)){

        if (system.countdown_duration == 0){
          system.countdown_duration = COUNTDOWN_DURATION;
          system.countdown_value    = 0;
        }

        if (system.countdown_duration > 0){
          bool update = false;
          if (system.countdown_duration < 1 && system.countdown_value){
            text::set_string(game.hud, system.countdown_text, "GO !");
            system.countdown_value = 0;
            update = true;
          }
          else if (system.countdown_value != (u32)system.countdown_duration)
          {
            system.countdown_value = (u32)system.countdown_duration;
            char tmp[32];
            sprintf(tmp, "%u", system.countdown_value);
            text::set_string(game.hud, system.countdown_text, tmp);
            update = true;
          }          

          if(update){
            glm::vec2 size;
            text::get_size(game.hud, system.countdown_text, size);

            glm::vec3 p(-size.x * 2, size.y * .5f, 0);
            text::set_local_position(game.hud, system.countdown_text, p);
          }
          
          system.countdown_duration -= delta_time;
        }
        else if (system.countdown_duration < 0) {
          text::set_string(game.hud, system.countdown_text, NULL);
          system.countdown_duration = 0;
          spawn();
        }

        return;
      }

      // handles auto increment velocity & split
      glm::vec3 p, v, valt;
      Hash<Ball>::Entry *begin, *end = hash::end(*system.balls);
      for (begin = hash::begin(*system.balls); begin < end; begin++) {
        Ball &ball = begin->value;
        if (ball.duration < BALL_ACC_TICK) {
          ball.duration += delta_time;
          continue;
        }

        if (ball.acc >=  BALL_MAX_ACC) {
          if (hash::size(*system.balls) >= NUM_BALL_SPLIT_MAX) continue;
          actor::get_velocity(game.world, ball.actor, v);
          actor::get_local_position(game.world, ball.actor, p);
          ball.duration = ball.acc = 0;

          v = glm::normalize(v) * BALL_DEFAULT_SPEED;

          // sets split
          valt = v;
          v.x *= -1;
          valt.y *= -1;

          actor::set_velocity(game.world, ball.actor, v);
          spawn_ball(p, valt);

          continue;
        }

        ball.duration = 0;
        actor::get_velocity(game.world, ball.actor, v);
        v *= BALL_ACC_INCR;
        actor::set_velocity(game.world, ball.actor, v);
        ball.acc++;
      }
    }

    void despawn(u64 unit)
    {
      //Ball *ball = hash::get(*system.balls, unit);
      //actor::set_touched_callback(game.world, ball->actor, NULL, NULL); <<---- chelou

      hash::remove(*system.balls, unit);

      world::despawn_unit(game.world, unit);
    }

    void spawn(void)
    {
      f32 vx = (randomize(0, 1) ? 1 : -1)*(.5f + rand_variance(BALL_SPAWN_ANGLE_VARIANCE));
      f32 vy = (randomize(0, 1) ? 1 : -1)*(1 - (vx < 0.f ? -vx : vx));
      glm::vec3 v(vx, vy, 0);

      v *= BALL_DEFAULT_SPEED;

      spawn_ball(IDENTITY_TRANSLATION, v);
    }
    
    void clear(void)
    {
      Hash<Ball>::Entry *begin, *end = hash::end(*system.balls);
      for (begin = hash::begin(*system.balls); begin < end; begin++)
        world::despawn_unit(game.world, begin->key);

      hash::clear(*system.balls);
    }

    // users bonus effects on balls funcs

    void split(void)
    {
      if (hash::size(*system.balls) > NUM_BALL_SPLIT_MAX) return;
      
      glm::vec3 p, v, valt;

      Hash<Ball>::Entry *begin, *end = hash::end(*system.balls);
      for (begin = hash::begin(*system.balls); begin < end; begin++) {
        Ball &ball = begin->value;

        actor::get_velocity(game.world, ball.actor, v);
        actor::get_local_position(game.world, ball.actor, p);

        // sets split
        valt = v;
        v.x *= -1;
        valt.y *= -1;

        actor::set_velocity(game.world, ball.actor, v);
        spawn_ball(p, valt, ball.duration, ball.acc);
      }
    }

    void nuke(void)
    {
      clear();
      spawn();
    }

    void disco(void)
    {
      printf("TODO: disco\n");
      // disco graphical effect
    }

    void accelerate(void)
    {
      glm::vec3 v;
      Hash<Ball>::Entry *begin, *end = hash::end(*system.balls);
      for (begin = hash::begin(*system.balls); begin < end; begin++) {
        Ball &ball = begin->value;
        actor::get_velocity(game.world, ball.actor, v);
        v *= BALL_BONUS_ACC_FACTOR;
        actor::set_velocity(game.world, ball.actor, v);
      }
    }
  
  }
}