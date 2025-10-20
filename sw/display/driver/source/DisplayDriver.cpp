#include <thread>
#include <chrono>

#include "driver/spi_master.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"

#include "DisplayDriver.h"
#include "Log.h"

namespace display::driver
{

static const char* TAG = "DISPLAYDD";
static constexpr uint16_t DISPLAY_WIDTH = 320;
static constexpr uint16_t DISPLAY_HEIGHT = 480;
static constexpr uint32_t SEND_BUF_SIZE = ((DISPLAY_WIDTH * DISPLAY_HEIGHT \
                * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) / 10);

DisplayDriver::DisplayDriver(drivers::gpio::GPIOProvider& gpioProvider):
m_power(nullptr),
m_reset(nullptr),
m_display(nullptr)
{
   m_power = gpioProvider.create(drivers::gpio::GPIO::PWR_DISPLAY_EN);
   m_reset = gpioProvider.create(drivers::gpio::GPIO::DISPLAY_RST);
   m_backlight = gpioProvider.create(drivers::gpio::GPIO::DISPLAY_LED_PWM);
   assert(m_power != nullptr);
   assert(m_reset != nullptr);
   assert(m_backlight != nullptr);

   drivers::gpio::Config config = {};
   config.direction = drivers::gpio::Direction::OUTPUT;
   config.state = drivers::gpio::State::LOW;
   assert(m_power->configure(config));

   drivers::gpio::Config resetConfig = {};
   resetConfig.direction = drivers::gpio::Direction::OUTPUT;
   resetConfig.state = drivers::gpio::State::HIGH;
   assert(m_reset->configure(resetConfig));

   drivers::gpio::Config backlightConfig = {};
   backlightConfig.direction = drivers::gpio::Direction::OUTPUT;
   backlightConfig.state = drivers::gpio::State::HIGH;
   assert(m_backlight->configure(backlightConfig));

   setPower(false);
   AIO_LOGI(TAG, "DisplayDriver created");
}

bool DisplayDriver::initialize()
{
   spi_bus_config_t buscfg = {
      .mosi_io_num = static_cast<int>(drivers::gpio::GPIO::DISPLAY_MOSI),
      .miso_io_num = static_cast<int>(drivers::gpio::GPIO::DISPLAY_MISO),
      .sclk_io_num = static_cast<int>(drivers::gpio::GPIO::DISPLAY_CLK),
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = SEND_BUF_SIZE + 8, //30720 + 8 = 30728
      .flags = SPICOMMON_BUSFLAG_MASTER,
   };
   if (spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK)
   {
       AIO_LOGE(TAG, "Failed to initialize SPI bus");
       return false;
   }

   esp_lcd_panel_io_handle_t ioHandle = nullptr;
   esp_lcd_panel_io_spi_config_t ioConfig = {
      .cs_gpio_num = static_cast<int>(drivers::gpio::GPIO::DISPLAY_CS),
      .dc_gpio_num = static_cast<int>(drivers::gpio::GPIO::DISPLAY_DATA_REG),
      .spi_mode = 0,
      .pclk_hz = 40000000,
      .trans_queue_depth = 17,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      };
   if (esp_lcd_new_panel_io_spi(SPI3_HOST, &ioConfig, &ioHandle) != ESP_OK)
   {
      AIO_LOGE(TAG, "Cannot create SPI IO panel!");
      return false;
   }

	esp_lcd_panel_handle_t panelHandle = nullptr;
	esp_lcd_panel_dev_config_t panelConfig = {
			.reset_gpio_num = -1,
         .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
			.bits_per_pixel = 16,
		};
	if (esp_lcd_new_panel_st7796(ioHandle, &panelConfig, &panelHandle) != ESP_OK)
   {
      AIO_LOGE(TAG, "Cannot create display panel!");
      return false;
   }

   setPower(true);
   reset();
   m_backlight->setState(drivers::gpio::State::LOW);

   ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));
   ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panelHandle, true));
   const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
   if (lvgl_port_init(&lvgl_cfg) != ESP_OK)
   {
      AIO_LOGE(TAG, "Cannot init LVGL!");
      return false;
   }

   const lvgl_port_display_cfg_t disp_cfg = {
       .io_handle = ioHandle,
       .panel_handle = panelHandle,
       .buffer_size = (DISPLAY_HEIGHT * DISPLAY_WIDTH) / 10,
       .double_buffer = true,
       .hres = DISPLAY_WIDTH,
       .vres = DISPLAY_HEIGHT,
       .monochrome = false,
       .rotation = {
           .swap_xy = false,
           .mirror_x = false,
           .mirror_y = true,
       },
       .color_format = LV_COLOR_FORMAT_RGB565,
       .flags = {
           .sw_rotate = false,
           .swap_bytes = true,
       }
   };

   lvgl_port_lock(0);
   AIO_LOGE(TAG, "[before display add] Free heap: %zu, largest block: %zu", heap_caps_get_free_size(MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
   m_display = lvgl_port_add_disp(&disp_cfg);
   if (!m_display)
   {
      AIO_LOGE_IF(!m_display, TAG, "Error adding display to LVGL engine!");
      lvgl_port_unlock();
      return false;
   }
   lv_display_set_rotation(m_display, LV_DISPLAY_ROTATION_90);
   lvgl_port_unlock();

   return true;
}
void DisplayDriver::finalize()
{
   AIO_LOGI(TAG, "Finalizing display...");
   setPower(false);
   m_backlight->setState(drivers::gpio::State::HIGH);
   lvgl_port_lock(0);
   if (m_display)
   {
      lvgl_port_remove_disp(m_display);
      m_display = nullptr;
   }
   lvgl_port_unlock();
   AIO_LOGI(TAG, "Display finalized");
}
void DisplayDriver::reset()
{
   AIO_LOGI(TAG, "Resetting display...");
   m_reset->setState(drivers::gpio::State::LOW);
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   m_reset->setState(drivers::gpio::State::HIGH);
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   AIO_LOGI(TAG, "Display reset done");
}
void DisplayDriver::setPower(bool state)
{
   AIO_LOGD(TAG, "Setting display power to %s", state? "ON" : "OFF");
   m_power->setState(state? drivers::gpio::State::HIGH : drivers::gpio::State::LOW);
   std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

} // namespace display
