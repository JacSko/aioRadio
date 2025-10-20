#include <array>
#include <queue>

#include "IDeviceDriver.h"
#include "SPIDriver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "FirmwareLoader.h"

namespace tuner::devicedriver
{

class DeviceDriverImpl : public IDeviceDriver
{
public:
   DeviceDriverImpl(drivers::gpio::GPIOProvider& gpioProvider);
   ~DeviceDriverImpl();
   void task();
   void processStatus();
   void healthCheck();
private:

   struct ListenerItem
   {
      EventType type;
      EventHandler* handler;
   };

   bool initialize() override;
   void finalize() override;
   bool setMode(TunerType type) override;
   bool transmit(FrameTransmit&) override;
   void setEventHandler(EventType type, EventHandler* handler) override;
   void removeEventHandler(EventType type) override;

   bool createTask();
   void destroyTask();

   std::unique_ptr<drivers::spi::SPIDriver> m_spiDriver;
   std::unique_ptr<drivers::gpio::IGPIO> m_powerPin;
   std::unique_ptr<FirmwareLoader> m_firmwareLoader;
   TaskHandle_t m_task;
   SemaphoreHandle_t m_mutex;
   uint32_t m_lastStatus;
   uint32_t m_newEvents;
   FrameTransmit m_currentFrame;
   TaskHandle_t m_frameTask;
   SemaphoreHandle_t m_listenersMutex;
   std::array<ListenerItem, 20> m_eventListeners = {};
};

}
