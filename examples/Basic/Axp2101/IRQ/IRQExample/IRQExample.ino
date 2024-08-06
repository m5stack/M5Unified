#include <M5Unified.h>

static bool got_notif_flag = false;

void setup(void)
{
  M5.begin();
  M5.Power.Axp2101.disableIRQ(AXP2101_IRQ_ALL);
  M5.Power.Axp2101.clearIRQStatuses();
  M5.Power.Axp2101.enableIRQ(
      AXP2101_IRQ_BAT_CHG_UNDER_TEMP  | AXP2101_IRQ_BAT_CHG_OVER_TEMP |  // Battery temp in charging
      AXP2101_IRQ_VBUS_INSERT         | AXP2101_IRQ_VBUS_REMOVE          // Usb insert/remove
  );

  M5.Display.setTextSize(3);
}

void check_irq_statuses()
{
  M5.Power.Axp2101.getIRQStatuses();
  
  if(M5.Power.Axp2101.isBatChargerUnderTemperatureIrq())
  {               
    M5.Display.drawString("BatUnderTempCharge", 50, 120);
    got_notif_flag = true;
  }
  if(M5.Power.Axp2101.isBatChargerOverTemperatureIrq())
  {    
    M5.Display.drawString("BatOverTempCharge", 50, 120);
    got_notif_flag = true;
  }
  if(M5.Power.Axp2101.isVbusInsertIrq())
  {    
    M5.Display.drawString("Usb inserted", 50, 120);
    got_notif_flag = true;
  }
  if(M5.Power.Axp2101.isVbusRemoveIrq())
  {
    M5.Display.drawString("Usb removed", 50, 120);
    got_notif_flag = true;
  }
  
  M5.Power.Axp2101.clearIRQStatuses();
}

void refresh_display()
{
  static unsigned long started_time = 0;
  if(got_notif_flag == false) { return; }
  
  unsigned long now_time = millis();
  if(started_time == 0) 
  { 
    started_time = now_time; 
  }
  else if(now_time - started_time > 500) 
  {
    started_time = 0;
    M5.Display.fillScreen(BLACK);
    got_notif_flag = false;
  }
}

void loop(void)
{   
  M5.update();
  check_irq_statuses();
  refresh_display();
  vTaskDelay(50);
}

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
