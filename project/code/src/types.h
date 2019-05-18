#pragma once

#include <engine/pge_types.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <time.h>

namespace app
{
  using namespace pge;

  const glm::vec3 IDENTITY_TRANSLATION(0.f);
  const glm::quat IDENTITY_ROTATION(1.f, 0.f, 0.f, 0.f);
  const f32 ARENA_HEIGHT = 700.f;

  const u64 U64_MAX = (u64)-1;
  const u32 U32_MAX = (u32)-1;

  const f32 PI = 3.14159265359f;
  const f32 PI4 = PI * .25f;

  const f32 BONUS_EFFECT_DURATION = 5.f;
}



namespace app
{
  inline int randomize(int min, int max)
  {
    srand((u32)time(NULL));
    return (rand() % (max - min + 1)) + min;
  }

  inline f32 rand_variance(f32 max)
  {
    srand((u32)time(NULL));
    i32 sign = rand() % 2 ? 1 : -1;
    return sign * static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / max));
  }
}