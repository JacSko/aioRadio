#include "driver/gpio.h"

#include "Log.h"
#include "GPIOProvider.h"
#include "socGPIO.h"
#include "i2cGPIO.h"

static const char* TAG = "GPIOProvider";

namespace drivers::gpio {

GPIOProvider::GPIOProvider(const mcp23017::i2cConfig& config)
{
   esp_err_t result = gpio_install_isr_service(0);
   if (result != ESP_OK)
   {
      AIO_LOGE(TAG, "Cannot install isr service (%u)", static_cast<int>(result));
   }
   m_i2cDriver = mcp23017::IMCP23017::create(0, config);
   assert(m_i2cDriver != nullptr);
   if (!m_i2cDriver->probe())
   {
      AIO_LOGE(TAG, "Error probing i2c expander!");
   }
}
GPIOProvider::~GPIOProvider()
{
   m_i2cDriver.reset();
   gpio_uninstall_isr_service();
}
std::unique_ptr<IGPIO> GPIOProvider::create(GPIO gpio)
{
   uint16_t gpioValue = static_cast<uint16_t>(gpio);
   if (gpioValue >= SOC_GPIO0 && gpioValue < SOC_GPIO_MAX)
   {
      return std::make_unique<socGPIO>(gpioValue, toString(gpio));
   }
   else if (gpioValue > SOC_GPIO_MAX && gpioValue < I2C_GPIO_MAX)
   {
      return std::make_unique<i2cGPIO>(gpioValue, *m_i2cDriver, toString(gpio));
   }
   else
   {
      //invalid GPIO ID
      AIO_LOGE(TAG, "Invalid GPIO%u", static_cast<uint32_t>(gpio));
      return nullptr;
   }
}
mcp23017::IMCP23017& GPIOProvider::getDriver()
{
   AIO_ASSERT(m_i2cDriver, TAG,"Create i2cDriver first!");
   return *m_i2cDriver;
}

const char* GPIOProvider::toString(GPIO gpio)
{
   switch (gpio)
   {
      case GPIO::DISPLAY_LED_PWM: return "DISPLAY_LED_PWM";
      case GPIO::DISPLAY_CS: return "DISPLAY_CS";
      case GPIO::TUNER_MISO: return "TUNER_MISO";
      case GPIO::TUNER_MOSI: return "TUNER_MOSI";
      case GPIO::TUNER_CLK: return "TUNER_CLK";
      case GPIO::TUNER_CS: return "TUNER_CS";
      case GPIO::IR_IN: return "IR_IN";
      case GPIO::DISPLAY_DATA_REG: return "DISPLAY_DATA_REG";
      case GPIO::DISPLAY_CLK: return "DISPLAY_CLK";
      case GPIO::DISPLAY_MISO: return "DISPLAY_MISO";
      case GPIO::GPIO_I2C_SDA: return "GPIO_I2C_SDA";
      case GPIO::GPIO_I2C_SCL: return "GPIO_I2C_SCL";
      case GPIO::DISPLAY_MOSI: return "DISPLAY_MOSI";
      case GPIO::SOC_I2S_DOUT: return "SOC_I2S_DOUT";
      case GPIO::SOC_I2S_BCLK: return "SOC_I2S_BCLK";
      case GPIO::SOC_I2S_FS: return "SOC_I2S_FS";
      case GPIO::GPIO_I2C_INT: return "GPIO_I2C_INT";
      case GPIO::GPIO_I2C_RESET: return "GPIO_I2C_RESET";
      case GPIO::TUNER_INT: return "TUNER_INT";
      case GPIO::TUNER_RST: return "TUNER_RST";
      case GPIO::DISPLAY_RST: return "DISPLAY_RST";
      case GPIO::PWR_DISPLAY_EN: return "PWR_DISPLAY_EN";
      case GPIO::PWR_TUNER_EN: return "PWR_TUNER_EN";
      case GPIO::I2S_SOURCE_SEL: return "I2S_SOURCE_SEL";
      case GPIO::DAC_MUTE: return "DAC_MUTE";
      case GPIO::CNT_ENC_BTN: return "CNT_ENC_BTN";
      case GPIO::CNT_ENC_B: return "CNT_ENC_B";
      case GPIO::CNT_ENC_A: return "CNT_ENC_A";
      case GPIO::CNT_BTN: return "CNT_BTN";
      default: return "Unknown";
   }
}

}
