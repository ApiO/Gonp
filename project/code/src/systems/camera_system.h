#pragma once

#include <engine/pge_types.h>
#include <glm/glm.hpp>

namespace app
{
  using namespace pge;

  namespace camera_system
  {
    void init     (const glm::vec2 &resolution);
    void shutdown (void);
    void update   (f64 delta_time);
    u64  get_id   (void);

    void shake   (const glm::vec2 &dir, f32 frequency, f32 magnitude, f32 time, f32 radial_frequency = 0, f32 radial_magnitude = 0);
    void revert  (void);
    void invert  (void);
  }
}