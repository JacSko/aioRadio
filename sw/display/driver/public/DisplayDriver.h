#pragma once
#include <string>
#include <cstdint>
#include "lvgl.h"

#include "GPIOProvider.h"

namespace display::driver
{

class DisplayDriver
{
public:
   DisplayDriver(drivers::gpio::GPIOProvider& gpioProvider);
   bool initialize();
   void finalize();

private:
   std::unique_ptr<drivers::gpio::IGPIO> m_power;
   std::unique_ptr<drivers::gpio::IGPIO> m_reset;
   std::unique_ptr<drivers::gpio::IGPIO> m_backlight;
   lv_disp_t* m_display;
   lv_obj_t* m_splashScreen;
   lv_obj_t* m_mainScreen;

   void reset();
   void setPower(bool state);

};

}
