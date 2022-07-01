#include <thread>

#include <M5Unified.h>
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

void setup(void);
void loop(void);

static int loopThread(void*)
{
  setup();
  for (;;)
  {
    std::this_thread::yield();
    loop();
  }
}

int main(int, char**)
{
  std::thread sub_thread(loopThread, nullptr);
  for (;;)
  {
    std::this_thread::yield();
    lgfx::Panel_sdl::sdl_event_handler();
  }
}
