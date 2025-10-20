#include <thread>
#include <memory>
#include <chrono>
#include "Log.h"
#include "GPIOProvider.h"
#include "GPIOMapping.h"
#include "IDeviceDriver.h"
#include "TunerManager.h"
#include "IDABTuner.h"
#include "IFMTuner.h"
#include "UserControl.h"
#include "DisplayDriver.h"
#include "IAudioControl.h"

#include "esp_heap_caps.h"

static const char* TAG = "AIO_RADIO";
using namespace drivers;

class aioRadio : public drivers::encoder::IEncoderListener
{
public:
   aioRadio():
   m_provider(gpio::mcp23017::i2cConfig{
            .i2cAddress = 0x20,
            .clockSpeedHz = 400000,
            .sclPin = static_cast<uint16_t>(drivers::gpio::GPIO::GPIO_I2C_SCL),
            .sdaPin = static_cast<uint16_t>(drivers::gpio::GPIO::GPIO_I2C_SDA),
            .resetPin = static_cast<uint16_t>(drivers::gpio::GPIO::GPIO_I2C_RESET),
            .interruptPin = static_cast<uint16_t>(drivers::gpio::GPIO::GPIO_I2C_INT),
             }),
   m_displayDriver(m_provider)
   {

   }
   void onEvent(const drivers::encoder::IEncoderListener::Event& event) override
   {
      AIO_LOGE(TAG, "Encoder event received: type=%s, diff=%d", drivers::encoder::IEncoderListener::eventName(event.type), event.diff);
   }
   void run()
   {
      bool result = false;

      result = m_displayDriver.initialize();
      AIO_LOGI(TAG, "DisplayDriver init resultL: %d", result);

      m_mainGUI = display::gui::IMainGUI::create();
      m_mainGUI->initialize(display::gui::ViewType::DAB);
      AIO_LOGI(TAG, "MainGUI created!");

      m_tunerDriver = tuner::devicedriver::IDeviceDriver::create(m_provider);
      result = m_tunerDriver->initialize();
      AIO_LOGI(TAG, "Device driver init result: %d", result);

      m_dabTuner = tuner::dab::IDABTuner::create(*m_tunerDriver);
      AIO_LOGI(TAG, "DABTuner created!");

      m_fmTuner = tuner::fm::IFMTuner::create(*m_tunerDriver);
      AIO_LOGI(TAG, "FMTuner created!");

      m_audioControl = audio::IAudioControl::create(*m_tunerDriver);
      AIO_LOGI(TAG, "AudioControl created!");

      m_userControl = std::make_unique<control::UserControl>(m_provider.getDriver(), *m_mainGUI, *m_audioControl);
      AIO_LOGI(TAG, "UserControl created!");

      m_tunerManager = std::make_unique<tuner::manager::TunerManager>(*m_mainGUI, *m_dabTuner, *m_fmTuner, *m_audioControl);

      while(1)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
   }
private:
   gpio::GPIOProvider m_provider;
   display::driver::DisplayDriver m_displayDriver;
   std::unique_ptr<display::gui::IMainGUI> m_mainGUI;
   std::unique_ptr<audio::IAudioControl> m_audioControl;
   std::unique_ptr<control::UserControl> m_userControl;
   std::unique_ptr<tuner::devicedriver::IDeviceDriver> m_tunerDriver;
   std::unique_ptr<tuner::dab::IDABTuner> m_dabTuner;
   std::unique_ptr<tuner::fm::IFMTuner> m_fmTuner;
   std::unique_ptr<tuner::manager::TunerManager> m_tunerManager;
};

extern "C" void app_main(void )
{
   // AIO_LOGE(TAG, "[before aioReadio creation] Free heap: %zu, largest block: %zu", heap_caps_get_free_size(MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
   aioRadio radio;
   // AIO_LOGE(TAG, "[after aioRadio creation] Free heap: %zu, largest block: %zu", heap_caps_get_free_size(MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
   radio.run();
}
