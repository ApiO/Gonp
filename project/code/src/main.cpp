#include <string>
#include <engine/pge.h>
#include <runtime/memory.h>

using namespace pge;

namespace app
{
  void init();
  void update(f64 delta_time);
  void render();
  void shutdown();

  char data_path[_MAX_PATH];
}

inline void handle_args(int argc, char * argv[])
{
  if (argc == 1){
    strcpy(app::data_path, "../../../data");
    return;
  }

  strcpy(app::data_path, argv[1]);
}

int main(int argc, char * argv[])
{
  handle_args(argc, argv);

  memory_globals::init();

  Allocator *a = &memory_globals::default_allocator();
  {
    application::init(app::init, app::update, app::render, app::shutdown, app::data_path, *a);

    while (!application::should_quit())
      application::update();

    application::shutdown();
  }
  memory_globals::shutdown();
}