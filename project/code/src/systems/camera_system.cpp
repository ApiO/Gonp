#include "camera_system.h"

#include <math.h>
#include <engine/pge.h>
#include <glm/glm.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/random.hpp>
#include <runtime/array.h>
#include <game_types.h>

using namespace pge;

namespace app
{
  const f32 DEFAULT_DAMPING = 4.0f;
  const f32 DEFAULT_FOV = 45.0f;
  const f32 DEFAULT_NEAR = 0.2f;
  const f32 DEFAULT_FAR = 100000.0f;

  const f32 INVERT_MAX_HEIGHT = 600.f;
  const f32 INVERT_DURATION = 10.f;

  struct Shake
  {
    bool active;
    f64 effect_time;
    f64 effect_duration;
    f32 shake_freq;
    f32 shake_mag;
    f32 shake_rad_freq;
    f32 shake_rad_mag;
    glm::vec2 shake_dir;
  };


  struct Step{
    f64       time;
    glm::vec3 position;
  };

  struct Buff
  {
    u64 text;
    f64 duration;
  };

  struct Mover
  {
    bool  active;
    Step *steps;
    u32   num_points;
    f64   state;
  };

  struct CameraSystem {
    u64   id;
    f32   rfov;
    f32   ratio;
    f32   tz;
    Buff  invert;
    Buff  revert;
    Shake shake;
    Mover mover;
    glm::vec3 position; 
    glm::vec3 rotation; //eulers in rad

    bool update_position;
    bool update_rotation;
    bool reverted;
  };

  CameraSystem system;

  glm::vec3 PATHS[] = {
    glm::vec3(0, INVERT_MAX_HEIGHT, system.tz),
    glm::vec3(0, INVERT_MAX_HEIGHT, 0),
    glm::vec3(0, INVERT_MAX_HEIGHT, -system.tz)
  };

  typedef void (*Callback)(void);
}

//internals

namespace app
{
  inline void _shake(f64 dt)
  {
    glm::vec3 tp, tps, tpp;

    system.update_position = true;

    tp = glm::vec3(0, 0, system.position.z);

    if (system.shake.effect_time >= system.shake.effect_duration) {
      system.shake.active = false;
      dt *= 10; 
    }
    else
    {
      tpp = glm::vec3(system.shake.shake_dir, 0);
      tps = tpp * (f32)((system.shake.shake_mag * (system.shake.effect_duration - system.shake.effect_time) / system.shake.effect_duration) * sin(2 * PI * system.shake.effect_time * system.shake.shake_freq));

      if (system.shake.shake_rad_mag > 0 && system.shake.shake_rad_freq > 0) {
        tpp.x = tpp.y;
        tpp.y = system.shake.shake_dir.x;
        tps += tpp * (f32)((system.shake.shake_rad_mag * (system.shake.effect_duration - system.shake.effect_time) / system.shake.effect_duration) * cos(2 * PI * system.shake.effect_time * system.shake.shake_rad_freq));
      }
      system.shake.effect_time += dt;
    }
    
    tp.z = system.position.z;
    system.position += (tp - system.position) * DEFAULT_DAMPING * (f32)dt + tps;
  }

  inline void _follow_path(f64 dt)
  {
    system.update_position = true;
    system.update_rotation = true;

    if (system.mover.state >= system.mover.steps[system.mover.num_points - 1].time){
      system.position = system.mover.steps[system.mover.num_points - 1].position;
      system.mover.active = false;
      system.mover.state  = 0;
      return;
    }

    //u32 index = (u32)(((system.mover.num_points - 1) * system.mover.state) / system.mover.duration);

    //update orientation

    system.mover.state += dt;
  }

  inline void set_buff(Buff &buff)
  {
    buff.duration = buff.duration ? 0 : BONUS_EFFECT_DURATION;
    if (!buff.duration)
      text::set_string(game.hud, buff.text, NULL);
  }

  static void _revert()
  {
    system.rotation.z += PI;
    system.update_rotation = true;
  }

  static void _invert()
  {
    system.rotation.y += PI;
    system.update_rotation = true;

    system.position.z *= -1;
    system.update_position = true;
  }

  static void update_buff(f64 dt,  Buff &buff, const char *format, Callback func)
  {
    if (buff.duration > 0){
      char tmp[32];
      sprintf(tmp, format, buff.duration);
      text::set_string(game.hud, buff.text, tmp);

      glm::vec3 p;
      f32 w;
      text::get_width(game.hud, buff.text, w);
      text::get_local_position(game.hud, buff.text, p);
      p.x = -(w*.5f);
      text::set_local_position(game.hud, buff.text, p);

      buff.duration -= dt;
    }
    else if (buff.duration < 0) {
      text::set_string(game.hud, buff.text, NULL);
      buff.duration = 0;
      func();
    }
  }
}

namespace app
{
  namespace camera_system
  {
    void init(const glm::vec2 &resolution)
    {
      system.rfov = DEFAULT_FOV * (f32)(atan(1.0)*4.0) / 180.0f; // precise fov in radians
      system.tz   = resolution.y / (2 * tan(system.rfov / 2)); // z for 1:1 render scale

      system.position = glm::vec3(0, 0, system.tz);
      system.id       = world::spawn_camera(game.world, resolution.x / resolution.y, system.position, IDENTITY_ROTATION);
      system.ratio    = resolution.x / resolution.y;
      system.update_position = false;

      camera::set_projection_type(game.world, system.id, PROJECTION_PERSPECTIVE);
      camera::set_vertical_fov(game.world, system.id, DEFAULT_FOV);
      camera::set_near_range(game.world, system.id, DEFAULT_NEAR);
      camera::set_far_range(game.world, system.id, DEFAULT_FAR);

      system.mover.active = false;
      system.mover.state  = 0;

      system.invert.text     = world::spawn_text(game.hud, "fonts/kristen.itc.36/kristen.itc.36", NULL, TEXT_ALIGN_LEFT, glm::vec3(0, game.screen_height*.25f, 0.f), IDENTITY_ROTATION, 3.f, Color(255, 255, 255, 60));
      system.invert.duration = 0;
      system.revert.text     = world::spawn_text(game.hud, "fonts/kristen.itc.36/kristen.itc.36", NULL, TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION, 3.f, Color(255, 255, 255, 60));
      system.revert.duration = 0;
    }

    void shutdown()
    {
      world::despawn_camera(game.world, system.id);
    }

    void update(f64 delta_time)
    {
      if (system.invert.duration)
        update_buff(delta_time, system.invert, "I: %.3f", _invert);

      if (system.revert.duration)
        update_buff(delta_time, system.revert, "R: %.3f", _revert);

      if (system.mover.active)
        _follow_path(delta_time);

      if (system.shake.active)
        _shake(delta_time);

      if (system.update_position)
        camera::set_local_translation(game.world, system.id, system.position);

      if (system.update_rotation)
        camera::set_local_rotation(game.world, system.id, glm::quat(system.rotation));
    }

    u64 get_id(void)
    {
      return system.id;
    }

    void shake(const glm::vec2 &dir, f32 frequency, f32 magnitude, f32 time,
      f32 radial_frequency, f32 radial_magnitude)
    {
    //*
      system.shake.active          = true;
      system.shake.effect_time     = 0;
      system.shake.effect_duration = time;
      system.shake.shake_freq      = frequency;
      system.shake.shake_mag       = magnitude;
      system.shake.shake_dir       = glm::normalize(dir);
      system.shake.shake_rad_freq  = radial_frequency;
      system.shake.shake_rad_mag   = radial_magnitude;
    //*/
    }

    
    void revert()
    {
      set_buff(system.revert);
      _revert();
    }

    void invert()
    {
      set_buff(system.invert);
      _invert();
    }

  }
}