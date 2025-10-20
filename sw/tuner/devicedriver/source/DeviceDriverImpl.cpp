#include <cstring>
#include <algorithm>

#include "DeviceDriverImpl.h"
#include "GPIOMapping.h"
#include "StatusChecker.hpp"
#include "Log.h"


namespace tuner::devicedriver
{

static const char* TAG = "TUNERDD";
static const int TASK_STACK_SIZE = 4096;
static const int TASK_PRIORITY = 2;

std::unique_ptr<IDeviceDriver> IDeviceDriver::create(drivers::gpio::GPIOProvider& gpioProvider)
{
   return std::make_unique<DeviceDriverImpl>(gpioProvider);
}

static void taskFunction(void* arg)
{
   DeviceDriverImpl* dd = static_cast<DeviceDriverImpl*>(arg);
   dd->task();
}

DeviceDriverImpl::DeviceDriverImpl(drivers::gpio::GPIOProvider& gpioProvider):
m_spiDriver(nullptr),
m_powerPin(nullptr),
m_task(nullptr),
m_mutex(xSemaphoreCreateRecursiveMutex()),
m_currentFrame({}),
m_frameTask(nullptr),
m_listenersMutex(xSemaphoreCreateMutex())
{
   drivers::spi::SPIDriverConfig spiConfig = {};
   spiConfig.host = SPI2_HOST;
   spiConfig.misoPin = static_cast<int>(drivers::gpio::GPIO::TUNER_MISO);
   spiConfig.mosiPin = static_cast<int>(drivers::gpio::GPIO::TUNER_MOSI);
   spiConfig.sckPin = static_cast<int>(drivers::gpio::GPIO::TUNER_CLK);
   spiConfig.csPin = static_cast<int>(drivers::gpio::GPIO::TUNER_CS);
   spiConfig.interruptPin = drivers::gpio::GPIO::TUNER_INT;
   spiConfig.resetPin = drivers::gpio::GPIO::TUNER_RST;
   spiConfig.clockSpeedHz = 8000000;
   spiConfig.mode = 0;
   m_spiDriver = std::make_unique<drivers::spi::SPIDriver>(spiConfig, gpioProvider);
   AIO_LOGE_IF(!m_spiDriver, TAG, "Failed to create SPIDriver!");
   m_powerPin = gpioProvider.create(drivers::gpio::GPIO::PWR_TUNER_EN);
   AIO_LOGE_IF(!m_powerPin, TAG, "Failed to create power pin!");
   m_firmwareLoader = std::make_unique<FirmwareLoader>(*m_spiDriver);
   AIO_LOGE_IF(!m_firmwareLoader, TAG, "Failed to create FirmwareLoader!");
}
DeviceDriverImpl::~DeviceDriverImpl()
{
   finalize();
}
bool DeviceDriverImpl::initialize()
{
   if (!m_powerPin || !m_spiDriver || !m_firmwareLoader)
   {
      AIO_LOGE(TAG, "Preconditions not met for initialization!");
      return false;
   }

   drivers::gpio::Config powerPinConfig = drivers::gpio::Config{
                                               .direction = drivers::gpio::Direction::OUTPUT,
                                               .pullMode = drivers::gpio::PullMode::NONE,
                                               .state = drivers::gpio::State::LOW};
   if(!m_powerPin->configure(powerPinConfig))
   {
      AIO_LOGE(TAG, "Failed to configure power pin!");
      return false;
   };
   if (!m_powerPin->setState(drivers::gpio::State::HIGH))
   {
      AIO_LOGE(TAG, "Failed to enable tuner power!");
      return false;
   }
   vTaskDelay(pdMS_TO_TICKS(10));
   return true;
}
bool DeviceDriverImpl::createTask()
{
   AIO_LOGD(TAG, "Creating devicedriver task");
   BaseType_t result = xTaskCreate(taskFunction, "DD", TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task);
   AIO_LOGE_IF(result != pdPASS, TAG, "Failed to create interrupt task, error %d", result);
   return result == pdPASS;
}
void DeviceDriverImpl::destroyTask()
{
   AIO_LOGD(TAG, "Destroying interrupt task");
   if (m_task != nullptr)
   {
      vTaskDelete(m_task);
      m_task = nullptr;
   }
}
void DeviceDriverImpl::task()
{
   // wait for inteerupt here. If there is a transmission ongoing,
   // read requried amount of data from the tuner and notify other thread.
   // If this interrupt is not related to any particular transmission,
   // read the status word and check for events.
   static const uint16_t INTERRUPT_TIMEOUT = 5000;
   AIO_LOGI(TAG, "Starting DeviceDriver loop...");
   for (;;)
   {
      uint8_t statusBuffer [6] = {};
      if(m_spiDriver->waitForInterrupt(INTERRUPT_TIMEOUT))
      {
         xSemaphoreTakeRecursive(m_mutex, portMAX_DELAY);
         if (m_currentFrame.rxBuffer && m_currentFrame.rxSize)
         {
            if(m_spiDriver->read(m_currentFrame.rxBuffer, m_currentFrame.rxSize))
            {
               m_lastStatus = StatusChecker::toStatusWord(m_currentFrame.rxBuffer);
               if (StatusChecker::check(m_currentFrame.rxBuffer) == StatusChecker::Status::ERROR ||
                   m_lastStatus & static_cast<uint32_t>(m_currentFrame.eventType))
               {
                  xTaskNotifyGive(m_frameTask);
               }
            }
            else
            {
               AIO_LOGE(TAG, "Cannot read data from SPI!!");
            }
         }
         else
         {
            if(m_spiDriver->read(statusBuffer, 6))
            {
               m_lastStatus = StatusChecker::toStatusWord(statusBuffer);
            }
         }
         xSemaphoreGiveRecursive(m_mutex);
         m_newEvents ^= m_lastStatus;
         processStatus();
      }
      else
      {
         AIO_LOGW(TAG, "No interrupt in last %ums, reading status...", INTERRUPT_TIMEOUT);
         if(m_spiDriver->read(statusBuffer, 6))
         {
            m_lastStatus = StatusChecker::toStatusWord(statusBuffer);
            AIO_LOGW(TAG, "Status: %.4x", m_lastStatus);
            // Enable in the future (?)
            // m_newEvents ^= m_lastStatus;
            // processStatus();
         }
      }
   }
}
void DeviceDriverImpl::processStatus()
{
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   xSemaphoreTakeRecursive(m_mutex, portMAX_DELAY);
   for (auto& item : m_eventListeners)
   {
      if ((m_lastStatus & static_cast<uint32_t>(item.type)) &&
          (m_newEvents & static_cast<uint32_t>(item.type)) &&
           item.handler != nullptr)
      {
         item.handler->onEvent(item.type);
      }
   }
   m_newEvents = 0;
   xSemaphoreGiveRecursive(m_mutex);
   xSemaphoreGive(m_listenersMutex);
}
void DeviceDriverImpl::healthCheck()
{
   uint8_t txBuffer [2] = {0x08, 0x00};
   uint8_t rxBuffer [23];
   FrameTransmit frame = {};
   frame.txBuffer = txBuffer;
   frame.rxBuffer = rxBuffer;
   frame.txSize = 2;
   frame.rxSize = 23;
   frame.eventType = tuner::EventType::CTS;
   frame.eventTimeout = 1000;
   bool result = transmit(frame);
   if (result)
   {
      uint16_t deviceId = (rxBuffer[10] << 8) | rxBuffer[9];
      AIO_LOGI(TAG, "%s OK, device: Si%u", __func__, deviceId);
   }
   AIO_LOGE_IF(!result, TAG, "%s failed!", __func__);
}
void DeviceDriverImpl::finalize()
{
   destroyTask();
   if (m_powerPin)
   {
      m_powerPin->setState(drivers::gpio::State::LOW);
   }
}
bool DeviceDriverImpl::setMode(TunerType type)
{
   AIO_LOGI(TAG, "Setting tuner mode to %d", static_cast<int>(type));

   if (type != TunerType::FM && type != TunerType::DAB)
   {
      AIO_LOGE(TAG, "Unsupported tuner type %d", static_cast<int>(type));
      return false;
   }

   destroyTask();
   if (!m_firmwareLoader->boot(type))
   {
      AIO_LOGE(TAG, "Failed to boot tuner chipset in mode %u!", static_cast<unsigned>(type));
      return false;
   }
   return createTask();

}
bool DeviceDriverImpl::transmit(FrameTransmit& frame)
{
   assert(m_spiDriver);
   assert(frame.eventType == EventType::CTS || frame.eventType == EventType::STCINT);

   if (!frame.txBuffer || frame.txSize == 0 || !frame.rxBuffer || frame.rxSize == 0)
   {
      AIO_LOGE(TAG, "Invalid transmit arguments! txBuffer: %p, txSize: %zu, rxBuffer: %p, rxSize: %zu",
                 frame.txBuffer, frame.txSize, frame.rxBuffer, frame.rxSize);
      return false;
   }

   xSemaphoreTakeRecursive(m_mutex, portMAX_DELAY);
   m_currentFrame = frame;
   m_frameTask = xTaskGetCurrentTaskHandle();
   bool result = m_spiDriver->write(frame.txBuffer, frame.txSize);
   AIO_LOGE_IF(!result, TAG, "SPI write failed!");
   if (result)
   {
      xSemaphoreGiveRecursive(m_mutex);
      xTaskNotifyStateClear(m_task);
      int eventReady = ulTaskNotifyTake(true, pdMS_TO_TICKS(frame.eventTimeout));
      xSemaphoreTakeRecursive(m_mutex, portMAX_DELAY);
      AIO_LOGE_IF(!eventReady, TAG, "Response not received in %u ms!", frame.eventTimeout);
   }
   m_currentFrame = {};
   m_frameTask = nullptr;
   xSemaphoreGiveRecursive(m_mutex);
   return result;
}
void DeviceDriverImpl::setEventHandler(EventType type, EventHandler* handler)
{
   if (!handler)
   {
      AIO_LOGE(TAG, "Handler cannot be null!");
      return;
   }

   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   auto item = std::find_if(
       m_eventListeners.begin(),
       m_eventListeners.end(),
       [type](const ListenerItem& item) { return item.type == type || item.handler == nullptr; });

   AIO_LOGW_IF(item->handler != nullptr, TAG, "Overriding listener for EventType=%d", static_cast<int>(type));
   AIO_LOGW_IF(item == m_eventListeners.end(), TAG, "No space for new listener! Type=%d", static_cast<int>(type));

   if (item != m_eventListeners.end())
   {
      item->type = type;
      item->handler = handler;
      AIO_LOGI(TAG, "Listener set for EventType=%d", static_cast<int>(type));
   }
   xSemaphoreGive(m_listenersMutex);
}
void DeviceDriverImpl::removeEventHandler(EventType type)
{
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   auto item = std::find_if(
      m_eventListeners.begin(),
      m_eventListeners.end(),
      [type](const ListenerItem& item) { return item.type == type;});
   if (item != m_eventListeners.end())
   {
      item->type = static_cast<EventType>(0);
      item->handler = nullptr;
      AIO_LOGI(TAG, "Listener removed for EventType=%d", static_cast<int>(type));
   }
   xSemaphoreGive(m_listenersMutex);
}


}
