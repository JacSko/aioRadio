#include <vector>

#include "i2cGPIO.h"
#include "TableConverter.hpp"
#include "driver/gpio.h"
#include "Log.h"

namespace drivers::gpio
{
static const char* TAG = "i2cGPIO";
static const int TASK_STACK_SIZE = 4096;
static const int TASK_NOTIFY_WAIT_MS = 1000;
static const int TASK_PRIORITY = 2;
static const std::vector<std::pair<uint16_t, mcp23017::PIN>> g_pinMapping = {
       {I2C_GPIOA0, mcp23017::PIN::GPIOA0},
       {I2C_GPIOA1, mcp23017::PIN::GPIOA1},
       {I2C_GPIOA2, mcp23017::PIN::GPIOA2},
       {I2C_GPIOA3, mcp23017::PIN::GPIOA3},
       {I2C_GPIOA4, mcp23017::PIN::GPIOA4},
       {I2C_GPIOA5, mcp23017::PIN::GPIOA5},
       {I2C_GPIOA6, mcp23017::PIN::GPIOA6},
       {I2C_GPIOA7, mcp23017::PIN::GPIOA7},
       {I2C_GPIOB0, mcp23017::PIN::GPIOB0},
       {I2C_GPIOB1, mcp23017::PIN::GPIOB1},
       {I2C_GPIOB2, mcp23017::PIN::GPIOB2},
       {I2C_GPIOB3, mcp23017::PIN::GPIOB3},
       {I2C_GPIOB4, mcp23017::PIN::GPIOB4},
       {I2C_GPIOB5, mcp23017::PIN::GPIOB5},
       {I2C_GPIOB6, mcp23017::PIN::GPIOB6},
       {I2C_GPIOB7, mcp23017::PIN::GPIOB7}
};

static void i2c_gpio_listener(mcp23017::PIN pin, bool state , void* arg)
{
   i2cGPIO* gpio = static_cast<i2cGPIO*>(arg);
   gpio->onInterrupt(pin, state);
}

static void taskFunction(void* arg)
{
   i2cGPIO* gpio = static_cast<i2cGPIO*>(arg);
   gpio->task();
}

i2cGPIO::i2cGPIO(uint16_t id, mcp23017::IMCP23017& driver, const std::string&& name) :
m_driver(driver),
m_name(std::move(name)),
m_interruptEdge(InterruptEdge::BOTH),
m_lastInputState(false),
m_listener(nullptr),
m_task(nullptr),
m_queue(nullptr),
m_listenersMutex(xSemaphoreCreateMutex())
{
   AIO_LOGI(TAG, "[%s] creating", m_name.c_str());
   assert(m_listenersMutex != nullptr);
   auto pinOpt = utilities::TableConverter::convert(g_pinMapping, id);
   if (!pinOpt.has_value())
   {
      AIO_LOGE(TAG, "[%s] Invalid I2C GPIO ID", m_name.c_str());
      assert(false);
   }
   m_id = pinOpt.value();
}
i2cGPIO::~i2cGPIO()
{
   AIO_LOGI(TAG, "[%s] destroying", m_name.c_str());
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   disableInterrupts();
   xSemaphoreGive(m_listenersMutex);
   destroyTask();
   destroyQueue();
}
bool i2cGPIO::isConfigValid(const Config& config)
{
   if (config.listener &&
       config.direction != Direction::INPUT)
   {
       AIO_LOGE(TAG, "[%s] can't set interrupt handler for output!", m_name.c_str());
       return false;
   }
   if (config.pullMode == PullMode::PULL_DOWN)
   {
       AIO_LOGE(TAG, "[%s] PULL_DOWN not supported by I2C expander", m_name.c_str());
       return false;
   }
   return true;
}
bool i2cGPIO::configure(const Config& config)
{
   if (!isConfigValid(config))
   {
      AIO_LOGE(TAG, "[%s] invalid configuration", m_name.c_str());
      return false;
   }
   if (!setDirection(config.direction))
   {
      AIO_LOGE(TAG, "[%s] Failed to set direction", m_name.c_str());
      return false;
   }
   if (!setPullMode(config.pullMode))
   {
      AIO_LOGE(TAG, "[%s] Failed to set pull mode", m_name.c_str());
      return false;
   }
   if (config.direction == Direction::OUTPUT)
   {
      if (!setState(config.state))
      {
         AIO_LOGE(TAG, "[%s] Failed to set state", m_name.c_str());
         return false;
      }
   }
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   if(config.listener)
   {
      m_listener = config.listener;
      if (!enableInterrupts(config.listener, config.interruptEdge))
      {
         AIO_LOGE(TAG, "[%s] Failed to enable interrupts", m_name.c_str());
         xSemaphoreGive(m_listenersMutex);
         return false;
      }
   }
   else
   {
      m_listener = nullptr;
      if (!disableInterrupts())
      {
         AIO_LOGE(TAG, "[%s] Failed to disable interrupts", m_name.c_str());
         xSemaphoreGive(m_listenersMutex);
         return false;
      }
   }
   xSemaphoreGive(m_listenersMutex);
   return true;
}
void i2cGPIO::onInterrupt(mcp23017::PIN pin, bool state)
{
   std::pair<mcp23017::PIN, bool> event = std::make_pair(pin, state);
   xQueueSend(m_queue, &event, 0);
}
void i2cGPIO::task()
{
   for (;;)
   {
      handleInterrupt();
   }
}
void i2cGPIO::handleInterrupt()
{
   std::pair<mcp23017::PIN, bool> event;
   if (xQueueReceive(m_queue, &event, pdMS_TO_TICKS(TASK_NOTIFY_WAIT_MS)) == pdTRUE)
   {
      AIO_LOGV(TAG, "[%s] interrupt received, state: %s", m_name.c_str(),
                                                          event.second ? "HIGH" : "LOW");
      xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
      if (m_listener && isCorrectEdge(event.second))
      {
         State gpioState = event.second ? State::HIGH : State::LOW;
         std::optional<uint16_t> gpioIdOpt = utilities::TableConverterReversed::convert(g_pinMapping, event.first);
         if (!gpioIdOpt.has_value())
         {
            AIO_LOGE(TAG, "[%s] Failed to convert MCP23017 PIN to GPIO ID (PIN %d)", m_name.c_str(),
                                                                                            static_cast<uint32_t>(event.first));
            return;
         }
         m_listener->onInterrupt(static_cast<GPIO>(*gpioIdOpt), gpioState);
      }
      m_lastInputState = event.second;
      xSemaphoreGive(m_listenersMutex);
   }
}
bool i2cGPIO::isCorrectEdge(bool state)
{
   bool isRising = (!m_lastInputState && state);
   bool isFalling = (m_lastInputState && !state);
   switch (m_interruptEdge)
   {
      case InterruptEdge::RISING:
         return isRising;
      case InterruptEdge::FALLING:
         return isFalling;
      case InterruptEdge::BOTH:
         return isRising || isFalling;
      default:
         return false;
   }
}
bool i2cGPIO::setState(State state)
{
   AIO_LOGD(TAG, "[%s] setting state to %s", m_name.c_str(), (state == State::HIGH) ? "HIGH" : "LOW");
   return m_driver.writePin(m_id, state == State::HIGH);
}
std::optional<State> i2cGPIO::getState()
{
   std::optional<bool> pinState = m_driver.readPin(m_id);
   if (!pinState.has_value())
   {
      AIO_LOGE(TAG, "[%s] Failed to read state", m_name.c_str());
      return std::nullopt;
   }
   AIO_LOGD(TAG, "[%s] read state %s", m_name.c_str(), pinState.value() ? "HIGH" : "LOW");
   return pinState.value() ? State::HIGH : State::LOW;
}
bool i2cGPIO::enableInterrupts(GPIOListener* listener, InterruptEdge edge)
{
   AIO_LOGD(TAG, "[%s] enabling interrupts", m_name.c_str());
   if (m_config.direction != Direction::INPUT)
   {
      AIO_LOGE(TAG, "[%s] interrupt can only be configured for input direction", m_name.c_str());
      return false;
   }
   if (listener == nullptr)
   {
      AIO_LOGE(TAG, "[%s] interrupt listener is null", m_name.c_str());
      return false;
   }
   m_listener = listener;
   m_interruptEdge = edge;
   m_driver.setInterruptCompare(m_id, mcp23017::InterruptCompare::COMPARE_PREVIOUS);
   m_driver.setListener(m_id, i2c_gpio_listener, this);
   if (!createQueue())
   {
      AIO_LOGE(TAG, "[%s] Failed to create interrupt queue", m_name.c_str());
      return false;
   }
   if (!createTask())
   {
      AIO_LOGE(TAG, "[%s] Failed to create interrupt task, destroying queue", m_name.c_str());
      destroyQueue();
      return false;
   }
   auto gpioState = getState();
   if (gpioState.has_value())
   {
      m_lastInputState = (gpioState.value() == State::HIGH);
   }
   return true;
}
bool i2cGPIO::createQueue()
{
   m_queue = xQueueCreate(10, sizeof(std::pair<mcp23017::PIN, bool>));
   if (m_queue == nullptr)
   {
      AIO_LOGE(TAG, "[%s] Failed to create interrupt queue", m_name.c_str());
      return false;
   }
   AIO_LOGD(TAG, "[%s] interrupt queue created", m_name.c_str());
   return true;
}
void i2cGPIO::destroyQueue()
{
   AIO_LOGD(TAG, "[%s] destroying interrupt queue", m_name.c_str());
   if (m_queue != nullptr)
   {
      vQueueDelete(m_queue);
      m_queue = nullptr;
   }
}
void i2cGPIO::destroyTask()
{
   AIO_LOGD(TAG, "[%s] destroying interrupt task", m_name.c_str());
   if (m_task != nullptr)
   {
      vTaskDelete(m_task);
      m_task = nullptr;
   }
}
bool i2cGPIO::createTask()
{
   AIO_LOGD(TAG, "[%s] creating interrupt task", m_name.c_str());
   char taskName[16];
   snprintf(taskName, sizeof(taskName), "i2cGPIO_%u", static_cast<int>(m_id));
   BaseType_t result = xTaskCreate(taskFunction, taskName, TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task);
   AIO_LOGE_IF(result != pdPASS, TAG, "[%s] Failed to create interrupt task, error %d", m_name.c_str(), result);
   return result == pdPASS;
}
bool i2cGPIO::disableInterrupts()
{
   AIO_LOGD(TAG, "[%s] disabling interrupts", m_name.c_str());
   m_driver.removeListener(m_id);
   m_listener = nullptr;
   return true;
}
bool i2cGPIO::setPullMode(PullMode mode)
{
   if (mode == PullMode::PULL_DOWN)
   {
      AIO_LOGE(TAG, "[%s] PullMode::PULL_DOWN not supported by I2C expander", m_name.c_str());
      return false;
   }
   return m_driver.setPullup(m_id, mode == PullMode::PULL_UP ? mcp23017::Pullup::ENABLED : mcp23017::Pullup::DISABLED);
}
bool i2cGPIO::setDirection(Direction direction)
{
   return m_driver.setDirection(m_id, direction == Direction::INPUT ? mcp23017::Direction::INPUT : mcp23017::Direction::OUTPUT);
}

} // namespace drivers::gpio

