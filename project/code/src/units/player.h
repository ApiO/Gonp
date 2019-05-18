#pragma once

#include <engine/pge_types.h>
#include <runtime/memory_types.h>
#include <runtime/collection_types.h>
#include <glm/glm.hpp>

namespace app
{
  using namespace pge;

  enum BonusType
  {
    BONUS_INVERT = 0,     // I
    BONUS_REVERT,         // R
    BONUS_DISCO,          // D
    BONUS_SPLIT,          // S  
    BONUS_ACCELERATE,     // A
    BONUS_NUKE,           // N
    BONUS_CONTRACT,       // C
    BONUS_EXTEND,         // E
    BONUS_COUNT
  };

  struct Bonus
  {
    BonusType type;
    u64 id;
  };

  typedef void(*Contract) (u32 index);

  struct Bouncer
  {
    Bouncer();
    u64 id;
    f32  scale_y;
    u64  scale_cooldown_text;
    f64  scale_cooldown_timer;
    f32  max_y;
    f32  min_y;
    Contract on_contract;
  };
  inline Bouncer::Bouncer() : on_contract(NULL) {};

  struct BonusBar
  {
    BonusBar();
    i32  selected_index;
    f64  last_selection;
    f64  last_action;
    bool add_bonus;
    Array<Bonus> *items;
    Array<u64>   *rm;
  };
  inline BonusBar::BonusBar() : items(NULL), rm(NULL){}

  struct Score
  {
    u64  id;
    u32  num_points;
    bool update;
  };

  class Player
  {
  public:
    void init       (glm::vec3 &position, u32 index, Allocator &a);
    void update     (f64 delta_time);
    void destroy    (void);
    void add_points (u32 num);
    void add_bonus  (void);
    void contract   (void);
  private:
    u32       index;
    i32       pad_index;
    glm::vec3 start_position;
    glm::vec3 position;
    u64       goal;
    Score     score;
    Bouncer   bouncer;
    BonusBar  bonus_bar;
    void handle_move      (f64 delta_time);
    void handle_use       (f64 delta_time);
    void handle_selection (f64 delta_time); 
    void run_bonus        (BonusType type);
    void change_scale     (f32 value);
  };

}