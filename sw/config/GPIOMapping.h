#pragma once
#include "GPIODefs.h"

namespace drivers::gpio {
   enum class GPIO : uint16_t
   {
      ONBOARD_LED      = SOC_GPIO2,
      ONBOARD_BTN      = SOC_GPIO0,

      DISPLAY_LED_PWM  = SOC_GPIO4,
      DISPLAY_CS       = SOC_GPIO5,
      TUNER_MISO       = SOC_GPIO12,
      TUNER_MOSI       = SOC_GPIO13,
      TUNER_CLK        = SOC_GPIO14,
      TUNER_CS         = SOC_GPIO15,
      IR_IN            = SOC_GPIO16,
      DISPLAY_DATA_REG = SOC_GPIO17,
      DISPLAY_CLK      = SOC_GPIO18,
      DISPLAY_MISO     = SOC_GPIO19,
      GPIO_I2C_SDA     = SOC_GPIO21,
      GPIO_I2C_SCL     = SOC_GPIO22,
      DISPLAY_MOSI     = SOC_GPIO23,
      SOC_I2S_DOUT     = SOC_GPIO25,
      SOC_I2S_BCLK     = SOC_GPIO26,
      SOC_I2S_FS       = SOC_GPIO27,
      GPIO_I2C_INT     = SOC_GPIO32,
      GPIO_I2C_RESET   = SOC_GPIO33,
      TUNER_INT        = SOC_GPIO34,
      TUNER_RST        = I2C_GPIOB0,
      DISPLAY_RST      = I2C_GPIOB1,
      PWR_DISPLAY_EN   = I2C_GPIOB2,
      PWR_TUNER_EN     = I2C_GPIOB3,
      I2S_SOURCE_SEL   = I2C_GPIOB4,
      DAC_MUTE         = I2C_GPIOB5,
      CNT_ENC_BTN      = I2C_GPIOB6,
      CNT_ENC_B        = I2C_GPIOA0,
      CNT_ENC_A        = I2C_GPIOA1,
      CNT_BTN          = I2C_GPIOA2
   };
}
