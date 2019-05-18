#pragma once

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <runtime/collection_types.h>

namespace app
{
  using namespace pge;

  namespace ball_system
  {
    void init     (Allocator &a);
    void shutdown (void);
    void update   (f64 delta_time);
    void despawn  (u64 unit);
    void spawn    (void);
    void clear    (void);

    // users bonus effects on balls funcs
    void split (void);
    void nuke  (void);
    void disco (void);
    void accelerate (void);
  }
}