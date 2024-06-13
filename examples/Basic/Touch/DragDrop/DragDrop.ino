#include <M5Unified.h>

struct box_t
{
  int x;
  int y;
  int w;
  int h;
  std::uint16_t color;
  int touch_id = -1;

  void clear(void)
  {
    M5.Display.setColor(M5.Display.getBaseColor());
    for (int i = 0; i < 8; ++i)
    {
      M5.Display.drawRect(x+i, y+i, w-i*2, h-i*2);
    }
  }
  void draw(void)
  {
    M5.Display.setColor(color);
    int ie = touch_id < 0 ? 4 : 8;
    for (int i = 0; i < ie; ++i)
    {
      M5.Display.drawRect(x+i, y+i, w-i*2, h-i*2);
    }
  }
  bool contain(int x, int y)
  {
    return this->x <= x && x < (this->x + this->w)
        && this->y <= y && y < (this->y + this->h);
  }
};

static constexpr std::size_t box_count = 4;
static box_t box_list[box_count];

void setup(void)
{
  M5.begin();

  for (std::size_t i = 0; i < box_count; ++i)
  {
    box_list[i].w = 100;
    box_list[i].h = 100;
  }
  box_list[0].x = 0;
  box_list[0].y = 0;
  box_list[1].x = M5.Display.width()  - box_list[1].w;
  box_list[1].y = 0;
  box_list[2].x = 0;
  box_list[2].y = M5.Display.height() - box_list[2].h;
  box_list[3].x = M5.Display.width()  - box_list[3].w;
  box_list[3].y = M5.Display.height() - box_list[3].h;
  box_list[0].color = TFT_RED;
  box_list[1].color = TFT_BLUE;
  box_list[2].color = TFT_GREEN;
  box_list[3].color = TFT_YELLOW;

  M5.Display.setEpdMode(epd_mode_t::epd_fastest);
  M5.Display.startWrite();
  M5.Display.setTextSize(2);
  for (std::size_t i = 0; i < box_count; ++i)
  {
    box_list[i].draw();
  }
}

void loop(void)
{
  M5.delay(1);

  M5.update();

  auto count = M5.Touch.getCount();
  if (!count)
  {
    return;
  }

  static m5::touch_state_t prev_state;
  auto t = M5.Touch.getDetail();
  if (prev_state != t.state)
  {
    prev_state = t.state;
    static constexpr const char* state_name[16] =
    { "none"
    , "touch" 
    , "touch_end" 
    , "touch_begin" 
    , "___" 
    , "hold" 
    , "hold_end" 
    , "hold_begin" 
    , "___" 
    , "flick"
    , "flick_end"
    , "flick_begin"
    , "___"
    , "drag"
    , "drag_end"
    , "drag_begin"
    };
    M5_LOGI("%s", state_name[t.state]);
  }

  for (std::size_t i = 0; i < count; ++i)
  {
    auto t = M5.Touch.getDetail(i);

    for (std::size_t j = 0; j < box_count; ++j)
    {
      if (t.wasHold())
      {
        if (box_list[j].contain(t.x, t.y))
        {
          box_list[j].touch_id = t.id;
        }
      }

      M5.Display.waitDisplay();
      if (box_list[j].touch_id == t.id)
      {
        if (t.wasReleased())
        {
          box_list[j].touch_id = -1;
          box_list[j].clear();
        }
        else
        {
          auto dx = t.deltaX();
          auto dy = t.deltaY();
          if (dx || dy)
          {
            box_list[j].clear();
            box_list[j].x += dx;
            box_list[j].y += dy;
          }
        }
      }
      box_list[j].draw();
    }
  }
  M5.Display.display();
}

#if !defined ( ARDUINO ) && defined ( ESP_PLATFORM )
extern "C" {
  void loopTask(void*)
  {
    setup();
    for (;;) {
      loop();
    }
    vTaskDelete(NULL);
  }

  void app_main()
  {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif
