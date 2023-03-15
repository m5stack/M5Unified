
#if defined ( ARDUINO )

#include <Arduino.h>

// If you use SD card, write this.
#include <SD.h>

// If you use SPIFFS, write this.
#include <SPIFFS.h>

#endif

// * The filesystem header must be included before the display library.

//----------------------------------------------------------------

// If you use ATOM Display, write this.
#include <M5AtomDisplay.h>

// If you use Module Display, write this.
#include <M5ModuleDisplay.h>

// If you use Module RCA, write this.
#include <M5ModuleRCA.h>

// If you use Unit GLASS, write this.
#include <M5UnitGLASS.h>

// If you use Unit OLED, write this.
#include <M5UnitOLED.h>

// If you use Unit LCD, write this.
#include <M5UnitLCD.h>

// If you use Unit RCA (for Video output), write this.
#include <M5UnitRCA.h>

// * The display header must be included before the M5Unified library.

//----------------------------------------------------------------

// Include this to enable the M5 global instance.
#include <M5Unified.h>


void setup(void)
{
  auto cfg = M5.config();

  // external display setting. (Pre-include required)
  cfg.external_display.module_display = true;  // default=true. use ModuleDisplay
  cfg.external_display.atom_display   = true;  // default=true. use AtomDisplay
  cfg.external_display.unit_glass     = true;  // default=true. use UnitGLASS
  cfg.external_display.unit_oled      = true;  // default=true. use UnitOLED
  cfg.external_display.unit_lcd       = true;  // default=true. use UnitLCD
  cfg.external_display.unit_rca       = true;  // default=true. use UnitRCA VideoOutput
  cfg.external_display.module_rca     = true;  // default=true. use ModuleRCA VideoOutput

/*
 Display with auto-detection
 - module_display
 - atom_display
 - unit_glass
 - unit_oled
 - unit_lcd

 Displays that cannot be auto-detected
 - module_rca
 - unit_rca

※ Note that if you enable a display that cannot be auto-detected, 
   it will operate as if it were connected, even if it is not actually connected.
   When RCA is enabled, it consumes a lot of memory to allocate the frame buffer.
//*/


// Set individual parameters for external displays.
// (※ Use only the items you wish to change. Basically, it can be omitted.)
#if defined ( __M5GFX_M5ATOMDISPLAY__ ) // setting for ATOM Display.
// cfg.atom_display.logical_width  = 1280;
// cfg.atom_display.logical_height = 720;
// cfg.atom_display.output_width   = 1280;
// cfg.atom_display.output_height  = 720;
// cfg.atom_display.refresh_rate   = 60;
// cfg.atom_display.scale_w        = 1;
// cfg.atom_display.scale_h        = 1;
// cfg.atom_display.pixel_clock    = 74250000;
#endif
#if defined ( __M5GFX_M5MODULEDISPLAY__ ) // setting for Module Display.
// cfg.module_display.logical_width  = 1280;
// cfg.module_display.logical_height = 720;
// cfg.module_display.output_width   = 1280;
// cfg.module_display.output_height  = 720;
// cfg.module_display.refresh_rate   = 60;
// cfg.module_display.scale_w        = 1;
// cfg.module_display.scale_h        = 1;
// cfg.module_display.pixel_clock    = 74250000;
#endif
#if defined ( __M5GFX_M5MODULERCA__ ) // setting for Module RCA.
// cfg.module_rca.logical_width  = 216;
// cfg.module_rca.logical_height = 144;
// cfg.module_rca.output_width   = 216;
// cfg.module_rca.output_height  = 144;
// cfg.module_rca.signal_type    = M5ModuleRCA::signal_type_t::PAL;     //  NTSC / NTSC_J / PAL_M / PAL_N
// cfg.module_rca.use_psram      = M5ModuleRCA::use_psram_t::psram_use; // psram_no_use / psram_half_use
// cfg.module_rca.pin_dac        = GPIO_NUM_26;
// cfg.module_rca.output_level   = 128;
#endif
#if defined ( __M5GFX_M5UNITRCA__ ) // setting for Unit RCA.
// cfg.unit_rca.logical_width  = 216;
// cfg.unit_rca.logical_height = 144;
// cfg.unit_rca.output_width   = 216;
// cfg.unit_rca.output_height  = 144;
// cfg.unit_rca.signal_type    = M5UnitRCA::signal_type_t::PAL;     //  NTSC / NTSC_J / PAL_M / PAL_N
// cfg.unit_rca.use_psram      = M5UnitRCA::use_psram_t::psram_use; // psram_no_use / psram_half_use
// cfg.unit_rca.pin_dac        = GPIO_NUM_26;
// cfg.unit_rca.output_level   = 128;
#endif
#if defined ( __M5GFX_M5UNITGLASS__ ) // setting for Unit GLASS.
// cfg.unit_glass.pin_sda  = GPIO_NUM_21;
// cfg.unit_glass.pin_scl  = GPIO_NUM_22;
// cfg.unit_glass.i2c_addr = 0x3D;
// cfg.unit_glass.i2c_freq = 400000;
// cfg.unit_glass.i2c_port = I2C_NUM_0;
#endif
#if defined ( __M5GFX_M5UNITOLED__ ) // setting for Unit OLED.
// cfg.unit_oled.pin_sda  = GPIO_NUM_21;
// cfg.unit_oled.pin_scl  = GPIO_NUM_22;
// cfg.unit_oled.i2c_addr = 0x3C;
// cfg.unit_oled.i2c_freq = 400000;
// cfg.unit_oled.i2c_port = I2C_NUM_0;
#endif
#if defined ( __M5GFX_M5UNITLCD__ ) // setting for Unit LCD.
// cfg.unit_lcd.pin_sda  = GPIO_NUM_21;
// cfg.unit_lcd.pin_scl  = GPIO_NUM_22;
// cfg.unit_lcd.i2c_addr = 0x3E;
// cfg.unit_lcd.i2c_freq = 400000;
// cfg.unit_lcd.i2c_port = I2C_NUM_0;
#endif


  // begin M5Unified.
  M5.begin(cfg);

  // Get the number of available displays
  int display_count = M5.getDisplayCount();

  for (int i = 0; i < display_count; ++i) {
  // All displays are available in M5.Displays.
  // ※ Note that the order of which displays are numbered is the order in which they are detected, so the order may change.

    int textsize = M5.Displays(i).height() / 60;
    if (textsize == 0) { textsize = 1; }
    M5.Displays(i).setTextSize(textsize);
    M5.Displays(i).printf("No.%d\n", i);
  }


// If an external display is to be used as the main display, it can be listed in order of priority.
  M5.setPrimaryDisplayType( {
      m5::board_t::board_M5ModuleDisplay,
      m5::board_t::board_M5AtomDisplay,
//    m5::board_t::board_M5ModuleRCA,
//    m5::board_t::board_M5UnitGLASS,
//    m5::board_t::board_M5UnitOLED,
//    m5::board_t::board_M5UnitLCD,
//    m5::board_t::board_M5UnitRCA,
  } );


  // The primary display can be used with M5.Display.
  M5.Display.print("primary display\n");


  // Examine the indexes of a given type of display
  int index_module_display = M5.getDisplayIndex(m5::board_t::board_M5ModuleDisplay);
  int index_atom_display = M5.getDisplayIndex(m5::board_t::board_M5AtomDisplay);
  int index_module_rca = M5.getDisplayIndex(m5::board_t::board_M5ModuleRCA);
  int index_unit_glass = M5.getDisplayIndex(m5::board_t::board_M5UnitGLASS);
  int index_unit_oled = M5.getDisplayIndex(m5::board_t::board_M5UnitOLED);
  int index_unit_lcd = M5.getDisplayIndex(m5::board_t::board_M5UnitLCD);
  int index_unit_rca = M5.getDisplayIndex(m5::board_t::board_M5UnitRCA);

  if (index_module_display >= 0) {
    M5.Displays(index_module_display).print("This is Module Display\n");
  }
  if (index_atom_display >= 0) {
    M5.Displays(index_atom_display).print("This is Atom Display\n");
  }
  if (index_module_rca >= 0) {
    M5.Displays(index_module_rca).print("This is Module RCA\n");
  }
  if (index_unit_glass >= 0) {
    M5.Displays(index_unit_glass).print("This is Unit GLASS\n");
  }
  if (index_unit_oled >= 0) {
    M5.Displays(index_unit_oled).print("This is Unit OLED\n");
  }
  if (index_unit_lcd >= 0) {
    M5.Displays(index_unit_lcd).print("This is Unit LCD\n");
  }
  if (index_unit_rca >= 0) {
    M5.Displays(index_unit_rca).print("This is Unit RCA\n");
  }
  vTaskDelay(5000);
}


// When creating a function for drawing, it can be used universally by accepting a LovyanGFX type as an argument.
void draw_function(LovyanGFX* gfx)
{
  int x = rand() % gfx->width();
  int y = rand() % gfx->height();
  int r = (gfx->width() >> 4) + 2;
  uint32_t c = rand();
  gfx->fillRect(x-r, y-r, r*2, r*2, c);
}


void loop(void)
{
  vTaskDelay(1);

  for (int i = 0; i < M5.getDisplayCount(); ++i) {
    int x = rand() % M5.Displays(i).width();
    int y = rand() % M5.Displays(i).height();
    int r = (M5.Displays(i).width() >> 4) + 2;
    uint32_t c = rand();
    M5.Displays(i).fillCircle(x, y, r, c);
  }

  for (int i = 0; i < M5.getDisplayCount(); ++i) {
    draw_function(&M5.Displays(i));
  }
}

// for ESP-IDF compat
#if !defined ( ARDUINO )
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
