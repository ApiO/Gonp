#include "player.h"

#include <engine/pge.h>
#include <runtime/memory.h>
#include <runtime/list.h>
#include <runtime/array.h>
#include <game_types.h>
#include <systems/camera_system.h>
#include <systems/ball_system.h>
#include <systems/arena_system.h>

using namespace pge;

// internals
namespace app
{
  //bouncer
  const f32 PAD_MOVE_SPEED     = 800.f;
  const f32 PAD_HALF_HEIGHT    = 90.f;
  const f32 COOLDOWN_TEXT_PAD  = 56.f;
  const glm::vec3 BONUS_OFFSET = glm::vec3(43.f, 0.f, 0.f);
  
  // goal
  const f32 GOAL_OFFSET      = 20.f;
  const glm::vec3 GOAL_SCALE = glm::vec3(1, 7, 1);

  // score
  const f32 SCORE_OFFSET_X = 20.f;
  const f32 SCORE_OFFSET_Y = 396.f;

  // bonus bar
  const u32 NUM_BONUSES_MAX       = 6;
  const f32 BONUS_TRANSX          = 580.f;
  const f32 BONUS_TRANSY          = 360.f;
  const f32 ARENA_HALF_HEIGHT     = 360.f;
  const f32 PAD_SENSITIVITY       = .7f;
  const f64 DELTA_SELECTION       = .1f;
  const f64 DELTA_ACTION          = .16f;

  const Color DEFAULT_COLOR(255.f, 255.f, 255.f, 255.f);
  const Color SELECTED_COLOR(255.f, 20.f, 20.f, 255.f);

  const char *BONUS_NAMES[] = {
    "units/bonus/invert",
    "units/bonus/revert",
    "units/bonus/disco",
    "units/bonus/split",
    "units/bonus/accelerate",
    "units/bonus/nuke",
    "units/bonus/contract",
    "units/bonus/extend"
  };

  const char *SPAWN_LEFT_ANIMATION_NAME  = "spawn_left";
  const char *SPAWN_RIGHT_ANIMATION_NAME = "spawn_right";
  const char *USE_ANIMATION_NAME         = "use";
  const char *REMOVE_ANIMATION_NAME      = "remove";
  const char *MOVE_LEFT_ANIMATION_NAME   = "move_left";
  const char *MOVE_RIGHT_ANIMATION_NAME  = "move_right";

  const KeyboardKey CONTROLS[2][6]{
      { KEYBOARD_KEY_Z, KEYBOARD_KEY_S, KEYBOARD_KEY_Q, KEYBOARD_KEY_D, KEYBOARD_KEY_SPACE, KEYBOARD_KEY_LEFT_CONTROL },
      { KEYBOARD_KEY_UP, KEYBOARD_KEY_DOWN, KEYBOARD_KEY_LEFT, KEYBOARD_KEY_RIGHT, KEYBOARD_KEY_RIGHT_CONTROL, KEYBOARD_KEY_KP_0 }
  };

  const Color PLAYERS_COLOR[2] ={
    Color(0, 208, 255, 255),
    Color(255, 180, 20, 255)
  };

  const glm::vec3 BONUS_POSITION[2] ={
    glm::vec3(-BONUS_TRANSX, BONUS_TRANSY, 0),
    glm::vec3(BONUS_TRANSX, BONUS_TRANSY, 0)
  };

  const char *GOAL_NAMES[2] = {
    "units/goal/goal1",
    "units/goal/goal2"
  };


  /*
  static void _dev_add_bonus(Array<Bonus> *bonuses, u32 index)
  {
    glm::vec3 position = BONUS_POSITION[index] + (index ? -1.f : 1.f)*(BONUS_OFFSET * (f32)(array::size(*bonuses)));;

    Bonus bonus;
    bonus.type = BONUS_EXTEND;//(BonusType)randomize(0, BONUS_COUNT - 1);
    bonus.id = world::spawn_unit(game.world, BONUS_NAMES[bonus.type], position, IDENTITY_ROTATION);

    unit::play_animation(game.world, bonus.id, index ? SPAWN_LEFT_ANIMATION_NAME : SPAWN_RIGHT_ANIMATION_NAME);

    array::push_back(*bonuses, bonus);
  }
  //*/
}


// class definitions
namespace app
{
  void Player::init(glm::vec3 &p, u32 i, Allocator &a)
  {
    glm::vec3 pos;
    glm::quat q;

    index             = i;
    pad_index         = i == 0 ? 1 : 2;
    start_position    = position = p;
    
    // bouncer
    bouncer.id          = world::spawn_unit(game.world, "units/player/player", p, IDENTITY_ROTATION);
    bouncer.on_contract = arena_system::contract_callback;
    bouncer.scale_y     = 2.f;
    bouncer.max_y       = p.y + ARENA_HALF_HEIGHT - PAD_HALF_HEIGHT;
    bouncer.min_y       = p.y - ARENA_HALF_HEIGHT + PAD_HALF_HEIGHT;
    q = glm::quat(glm::vec3(0, 0, PI*.5f * (p.x < 0 ? 1 : -1)));
    pos = p;
    pos.x += p.x < 0 ? -COOLDOWN_TEXT_PAD : COOLDOWN_TEXT_PAD;
    bouncer.scale_cooldown_text  = world::spawn_text(game.world, "fonts/kristen.itc.36/kristen.itc.36", NULL, TEXT_ALIGN_LEFT, pos, q, 1.f, PLAYERS_COLOR[index]);
    bouncer.scale_cooldown_timer = 0;
    
    u64 sprite = unit::sprite(game.world, bouncer.id, 0);
    sprite::set_color(game.world, sprite, PLAYERS_COLOR[index]);
    
    // bonus_bar
    bonus_bar.items          = MAKE_NEW(a, Array<Bonus>, a);
    bonus_bar.rm             = MAKE_NEW(a, Array<u64>, a);
    bonus_bar.selected_index = -1;
    bonus_bar.last_selection = 0;
    bonus_bar.last_action    = 0;
    bonus_bar.add_bonus      = true;


    // score
    score.num_points = 0;
    score.update     = false;

    pos = glm::vec3(index ? SCORE_OFFSET_X : -(SCORE_OFFSET_X + 35), SCORE_OFFSET_Y, 0);
    score.id = world::spawn_text(game.world, "fonts/kristen.itc.36/kristen.itc.36", index ? "0" : "0", TEXT_ALIGN_LEFT, pos, IDENTITY_ROTATION, 1.4f);
    text::set_color(game.world, score.id, PLAYERS_COLOR[index]);

    // goal
    pos = p;
    pos.x += GOAL_OFFSET * index ? 1 : -1;
    goal = world::spawn_unit(game.world, GOAL_NAMES[index], pos, IDENTITY_ROTATION, GOAL_SCALE);
    actor::set_touched_callback(game.world, unit::actor(game.world, goal, 0), arena_system::goal_recieves_balls, &index);


    //dev-------------------------------------------
    /*
    {
      for (i32 i = 0; i < NUM_BONUSES_MAX; i++)
        _dev_add_bonus(bonus_bar.items, index);
      bonus_bar.selected_index = 0;
      u64 sprite = unit::sprite(game.world, (*bonus_bar.items)[bonus_bar.selected_index].id, 0);
      sprite::set_color(game.world, sprite, SELECTED_COLOR);
    }
    //*/
    //dev-------------------------------------------
  }

  void Player::destroy(void)
  {
    world::despawn_text(game.world, score.id);
    world::despawn_text(game.world, bouncer.scale_cooldown_text);
    world::despawn_unit(game.world, bouncer.id);
    world::despawn_unit(game.world, goal);

    Allocator &a = *bonus_bar.rm->_allocator;
    MAKE_DELETE(a, Array<Bonus>, bonus_bar.items);
    MAKE_DELETE(a, Array<u64>, bonus_bar.rm);
  }


  void Player::contract(void)
  {
    bouncer.scale_cooldown_timer = BONUS_EFFECT_DURATION;

    if (bouncer.scale_y == .5f) return;
    
    change_scale(.5f);
  }
  
  void Player::add_bonus()
  {
    bonus_bar.add_bonus = true;
  }
  
  void Player::add_points(u32 num)
  {
    score.num_points += num;
    score.update = true;
  }


  void Player::update(f64 dt)
  {
    // handles score
    if (score.update){
      score.update = false;

      char buf[64];
      sprintf(buf, "%d", score.num_points);
      text::set_string(game.world, score.id, buf);

      if (!index){
        f32 width;
        text::get_width(game.world, score.id, width);
        glm::vec3 p(-(SCORE_OFFSET_X + width), SCORE_OFFSET_Y, 0);
        text::set_local_position(game.world, score.id, p);
      }
    }

    // despawn bonus if needed
    {
      Array<u64> &rm = *bonus_bar.rm;
      u32 i = 0;
      u32 len = array::size(rm);
      while (i < len){
        u64 id = rm[i];
        if (!unit::is_playing_animation(game.world, id)){
          world::despawn_unit(game.world, id);
          rm[i] = array::pop_back(rm);
          len--;
          continue;
        }
        i++;
      }
    }

    // handles move
    handle_move(dt);
    
    // handles bonus use
    if (bonus_bar.selected_index!= -1) handle_use(dt);

    // handles cooldown text
    if (bouncer.scale_cooldown_timer > 0){
      char buff[32];
      sprintf(buff, "%.3f", bouncer.scale_cooldown_timer);
      text::set_string(game.world, bouncer.scale_cooldown_text, buff);

      glm::vec3 p;
      f32 w;
      text::get_width(game.world, bouncer.scale_cooldown_text, w);
      text::get_local_position(game.world, bouncer.scale_cooldown_text, p);

      p.y = position.y + (w*.5f)*(p.x < 0 ? -1 : 1);
      //p.y = position.y + (w*.5f)*(p.x < 0 ? -1 : 1) - (PAD_HALF_HEIGHT*.5f);

      text::set_local_position(game.world, bouncer.scale_cooldown_text, p);
      
      bouncer.scale_cooldown_timer -= dt;
    }
    else if (bouncer.scale_cooldown_timer < 0) {
      text::set_string(game.world, bouncer.scale_cooldown_text, NULL);
      bouncer.scale_cooldown_timer = 0;
      change_scale(1.f);
    }

    // handles bonus navigation
    handle_selection(dt);
    
    // handles bonus spawn
    if (!bonus_bar.add_bonus) return;

    bonus_bar.add_bonus = false;
    Array<Bonus> &bonuses = *bonus_bar.items;
    if (array::size(bonuses) == NUM_BONUSES_MAX) return;

    glm::vec3 position = BONUS_POSITION[index] + (index ? -1.f : 1.f)*(BONUS_OFFSET * (f32)(array::size(bonuses)));;

    Bonus bonus;
    bonus.type = (BonusType)randomize(0, BONUS_COUNT - 1);
    bonus.id = world::spawn_unit(game.world, BONUS_NAMES[bonus.type], position, IDENTITY_ROTATION);

    unit::play_animation(game.world, bonus.id, index ? SPAWN_LEFT_ANIMATION_NAME : SPAWN_RIGHT_ANIMATION_NAME);

    array::push_back(bonuses, bonus);

    if (bonus_bar.selected_index == -1) {
      bonus_bar.selected_index = 0;
      u64 sprite = unit::sprite(game.world, bonuses[bonus_bar.selected_index].id, 0);
      sprite::set_color(game.world, sprite, SELECTED_COLOR);
    }

  }


  void Player::handle_move(f64 dt)
  {
    bool up = keyboard::button(CONTROLS[index][0]) ||
      (pad::active(pad_index) && pad::axes(pad_index, 1) < -PAD_SENSITIVITY);
    bool down = keyboard::button(CONTROLS[index][1]) ||
      (pad::active(pad_index) && pad::axes(pad_index, 1) > PAD_SENSITIVITY);

    if (!up && !down) return;

    if (down && position.y == bouncer.min_y) return;
    else if (up && position.y == bouncer.max_y) return;
    
    f32 acc = (f32)(PAD_MOVE_SPEED * dt);
    if (down) acc *= -1;

    f32 y = position.y + acc;
    
    position.y = y;

    if (down && position.y <= bouncer.min_y) position.y = bouncer.min_y;
    else if (up && position.y >= bouncer.max_y) position.y = bouncer.max_y;

    unit::set_local_position(game.world, bouncer.id, 0, position);
  }

  void Player::handle_use(f64 dt)
  {
    bool use = keyboard::button(CONTROLS[index][4]) ||
      (pad::active(pad_index) && pad::pressed(pad_index, PAD_KEY_1));
    bool remove = keyboard::button(CONTROLS[index][5]) ||
      (pad::active(pad_index) && pad::pressed(pad_index, PAD_KEY_2));

    if ((!use && !remove) || bonus_bar.last_action < DELTA_ACTION){
      bonus_bar.last_action += dt;
      return;
    }
    
    Array<Bonus> &arr = *bonus_bar.items;
    bonus_bar.last_action = 0;

    u64 id = arr[bonus_bar.selected_index].id;
    if(use) run_bonus(arr[bonus_bar.selected_index].type);

    bool last_deleted = (u32)bonus_bar.selected_index == array::size(arr)-1;

    // sorts bonuses and removes bonus
    if ((u32)bonus_bar.selected_index < array::size(arr) - 1)
      for (u32 i = bonus_bar.selected_index; i <= array::size(arr) - 2; i++) arr[i] = arr[i + 1];
    array::pop_back(arr);

    // play animation
    unit::play_animation(game.world, id, remove ? REMOVE_ANIMATION_NAME : USE_ANIMATION_NAME);

    // controls selection index
    if (!array::size(arr))
      bonus_bar.selected_index = -1;
    else if (bonus_bar.selected_index > 0 && (u32)bonus_bar.selected_index > array::size(arr) - 1)
      bonus_bar.selected_index--;
      
    // slide next bonuses
    u32 max_index = array::size(arr) - 1;
    if (bonus_bar.selected_index != -1 && !last_deleted && (u32)bonus_bar.selected_index <= max_index) {
      glm::vec3 p;
      for (u32 i = bonus_bar.selected_index; i <= max_index; i++){
        Bonus &bonus = arr[i];
        unit::get_world_position(game.world, bonus.id, 0, p);
        p += BONUS_OFFSET * (index ? 1.f : -1.f);
        unit::set_local_position(game.world, bonus.id, 0, p);
        unit::play_animation(game.world, bonus.id, index ? MOVE_RIGHT_ANIMATION_NAME : MOVE_LEFT_ANIMATION_NAME);
      }
    }

    // registers for auto despwan when anim ends
    array::push_back(*bonus_bar.rm, id);

    // handles graphical bonus focus
    if (array::size(arr)) {
      u64 sprite = unit::sprite(game.world, id, 0);
      sprite::set_color(game.world, sprite, DEFAULT_COLOR);

      sprite = unit::sprite(game.world, arr[bonus_bar.selected_index].id, 0);
      sprite::set_color(game.world, sprite, SELECTED_COLOR);
    }
  }

  void Player::handle_selection(f64 dt)
  {
    if (bonus_bar.selected_index == -1) return;

    bool left = keyboard::button(CONTROLS[index][2]) ||
      (pad::active(pad_index) && pad::axes(pad_index, 4) < -PAD_SENSITIVITY);
    bool right = keyboard::button(CONTROLS[index][3]) ||
      (pad::active(pad_index) && pad::axes(pad_index, 4) > PAD_SENSITIVITY);

    bool move = false;
    u32 last_index = bonus_bar.selected_index;

    if ((left || right) && bonus_bar.last_selection > DELTA_SELECTION){
      bonus_bar.last_selection = 0;
      if (index == 1){
        right = left;
        left = !left;
      }
      if (left){
        if (bonus_bar.selected_index == 0) return;
        bonus_bar.selected_index--;
        move = true;
      }
      if (right){
        if ((u32)bonus_bar.selected_index == array::size(*bonus_bar.items) - 1) return;
        bonus_bar.selected_index++;
        move = true;
      }
    }
    else
      bonus_bar.last_selection += dt;

    if (move){
      u64 last_focus = (*bonus_bar.items)[last_index].id;
      u64 sprite = unit::sprite(game.world, last_focus, 0);
      sprite::set_color(game.world, sprite, DEFAULT_COLOR);

      u64 focus = (*bonus_bar.items)[bonus_bar.selected_index].id;
      sprite = unit::sprite(game.world, focus, 0);
      sprite::set_color(game.world, sprite, SELECTED_COLOR);
    }
  }
  
  void Player::run_bonus(BonusType type)
  {
    switch (type)
    {
    case BONUS_INVERT: 
      camera_system::invert();
      camera_system::shake(glm::vec2(0, 1), 10.f, 40.f, .17f);
      break;
    case BONUS_REVERT:
      camera_system::revert();
      camera_system::shake(glm::vec2(0, 1), 10.f, 40.f, .17f);
      break;
    case BONUS_DISCO:      
      ball_system::disco(); 
      break;
    case BONUS_SPLIT:
      ball_system::split();
      break;
    case BONUS_ACCELERATE: 
      ball_system::accelerate();  
      break;
    case BONUS_NUKE:        
      ball_system::nuke(); 
      camera_system::shake(glm::vec2(0, 1), 10.f, 40.f, .17f);
      break;
    case BONUS_CONTRACT: 
      bouncer.on_contract(index);
      break;
    case BONUS_EXTEND:
      bouncer.scale_cooldown_timer = BONUS_EFFECT_DURATION;
      if (bouncer.scale_y == 2.f) return;
      change_scale(2.f);
      break;
    }
  }

  void Player::change_scale(f32 value)
  {
    bouncer.scale_y = value;;

    unit::set_local_scale(game.world, bouncer.id, 0, glm::vec3(1.f, bouncer.scale_y, 1.f));

    bouncer.max_y = start_position.y + ARENA_HALF_HEIGHT - PAD_HALF_HEIGHT * bouncer.scale_y;
    bouncer.min_y = start_position.y - ARENA_HALF_HEIGHT + PAD_HALF_HEIGHT * bouncer.scale_y;

    if (position.y < bouncer.min_y){
      position.y = bouncer.min_y;
      unit::set_local_position(game.world, bouncer.id, 0, position);
    }
    else if (position.y > bouncer.max_y){
      position.y = bouncer.max_y;
      unit::set_local_position(game.world, bouncer.id, 0, position);
    }
  }

}