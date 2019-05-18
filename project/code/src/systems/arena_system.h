#pragma once

#include <engine/pge.h>
#include <runtime/types.h>
#include <runtime/collection_types.h>
#include <runtime/memory_types.h>

namespace app
{
  namespace arena_system
  {
    using namespace pge;

    #define NUM_PLAYER_MAX 6u

    void init     (Allocator &a);
    void shutdown (void);
    void update   (f64 delta_time);
    
    void contract_callback   (u32 player);
    void goal_recieves_balls (const Array<ContactPoint> &contacts, const void *user_data);
  }
}