#include <vector>

#include "socGPIO.h"
#include "TableConverter.hpp"
#include "driver/gpio.h"
#include "Log.h"

namespace drivers::gpio
{

static const char* TAG = "socGPIO";
static const int TASK_STACK_SIZE = 4096;
static const int TASK_NOTIFY_WAIT_MS = 1000;
static const int TASK_PRIORITY = 2;

static void IRAM_ATTR gpio_soc_isr_handler(void* arg)
{
   socGPIO* gpio = static_cast<socGPIO*>(arg);
   gpio->onInterrupt();
}

static void taskFunction(void* arg)
{
   socGPIO* gpio = static_cast<socGPIO*>(arg);
   gpio->task();
   vTaskDelete(NULL);
}

socGPIO::socGPIO(uint16_t id, const std::string&& name) :
m_id(id),
m_name(std::move(name)),
m_task(nullptr),
m_listenersMutex(xSemaphoreCreateMutex())
{
   AIO_LOGI(TAG, "[%s] creating", m_name.c_str());
   assert(m_listenersMutex != nullptr);
   gpio_reset_pin(static_cast<gpio_num_t>(m_id));
}
socGPIO::~socGPIO()
{
   AIO_LOGI(TAG, "[%s] destroying", m_name.c_str());
   disableInterrupts();
   gpio_reset_pin(static_cast<gpio_num_t>(m_id));
}
void socGPIO::onInterrupt()
{
   //This method is called in ISR context
   ESP_DRAM_LOGE(DRAM_STR("SOC"), "task notify from ISR for GPIO %u", static_cast<unsigned int>(m_id));
   vTaskNotifyGiveFromISR(m_task, nullptr);
}
void socGPIO::task()
{
   for (;;)
   {
      int eventReady = ulTaskNotifyTake(true, pdMS_TO_TICKS(TASK_NOTIFY_WAIT_MS));
      if (eventReady)
      {
         handleInterrupt();
      }
   }
}
void socGPIO::handleInterrupt()
{
   AIO_LOGV(TAG, "[%s] interrupt received, reading state...", m_name.c_str());
   int level = gpio_get_level(static_cast<gpio_num_t>(m_id));
   AIO_LOGV(TAG, "[%s] is %u, calling listeners...", m_name.c_str(), level);
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   if (m_listener)
   {
      m_listener->onInterrupt(static_cast<GPIO>(m_id), static_cast<State>(level ?
                                                       State::HIGH :
                                                       State::LOW));
   }
   xSemaphoreGive(m_listenersMutex);
}
bool socGPIO::createTask()
{
   AIO_LOGI(TAG, "[%s] creating interrupt task", m_name.c_str());
   char taskName[16];
   snprintf(taskName, sizeof(taskName), "socGPIO_%u", static_cast<unsigned int>(m_id));
   BaseType_t result = xTaskCreate(taskFunction, taskName, TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task);
   AIO_LOGE_IF(result != pdPASS, TAG, "[%s] Failed to create interrupt task, error %d", m_name.c_str(), result);
   return result == pdPASS;
}
void socGPIO::destroyTask()
{
   AIO_LOGI(TAG, "[%s] destroying interrupt task", m_name.c_str());
   if (m_task != nullptr)
   {
      vTaskDelete(m_task);
      m_task = nullptr;
   }
}
bool socGPIO::isConfigValid(const Config& config)
{
   if (!GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(m_id)))
   {
      AIO_LOGE(TAG, "[%s] this is not a valid ID", m_name.c_str());
      return false;
   }
   if (config.direction == Direction::OUTPUT &&
       !GPIO_IS_VALID_OUTPUT_GPIO(static_cast<gpio_num_t>(m_id)))
   {
      AIO_LOGE(TAG, "[%s] cannot be configured as output", m_name.c_str());
      return false;
   }
   if (config.listener &&
       config.direction != Direction::INPUT)
   {
      AIO_LOGE(TAG, "[%s] interrupt can only be configured for input direction", m_name.c_str());
      return false;
   }
   return true;
}
bool socGPIO::configure(const Config& config)
{
   AIO_LOGI(TAG, "[%s] configuring", m_name.c_str());
   bool isValid = isConfigValid(config);
   if (isValid)
   {
      gpio_config_t io_conf = {};
      io_conf.pin_bit_mask = (1ULL << static_cast<uint32_t>(m_id));
      io_conf.mode = (config.direction == Direction::INPUT) ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT;
      io_conf.pull_up_en = (config.pullMode == PullMode::PULL_UP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
      io_conf.pull_down_en = (config.pullMode == PullMode::PULL_DOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
      io_conf.intr_type = GPIO_INTR_DISABLE;
      esp_err_t err = gpio_config(&io_conf);
      if (err != ESP_OK)
      {
         AIO_LOGE(TAG, "[%s] Failed to configure GPIO", m_name.c_str());
         return false;
      }
      if (config.direction == Direction::OUTPUT)
      {
         setState(config.state);
      }
      if (config.listener)
      {
         enableInterrupts(config.listener, config.interruptEdge);
      }
      else
      {
         disableInterrupts();
      }
      return true;
   }
   else
   {
      AIO_LOGE(TAG, "[%s] invalid configuration", m_name.c_str());
      return false;
   }
}
bool socGPIO::setState(State state)
{
   AIO_LOGD(TAG, "[%s] setting state to %s", m_name.c_str(), (state == State::HIGH) ? "HIGH" : "LOW");
   return gpio_set_level(static_cast<gpio_num_t>(m_id), (state == State::HIGH) ? 1 : 0) == ESP_OK;
}
std::optional<State> socGPIO::getState()
{
   int level = gpio_get_level(static_cast<gpio_num_t>(m_id));
   if (level < 0)
   {
      AIO_LOGE(TAG, "[%s] failed to get state", m_name.c_str());
      return std::nullopt;
   }
   return level? State::HIGH : State::LOW;
}
bool socGPIO::enableInterrupts(GPIOListener* listener, InterruptEdge edge)
{
   if (listener == nullptr)
   {
      AIO_LOGE(TAG, "[%s] interrupt listener is null", m_name.c_str());
      return false;
   }
   m_listener = listener;
   static const std::vector<std::pair<InterruptEdge, gpio_int_type_t>> edgeTable = {
       {InterruptEdge::RISING, GPIO_INTR_POSEDGE},
       {InterruptEdge::FALLING, GPIO_INTR_NEGEDGE},
       {InterruptEdge::BOTH, GPIO_INTR_ANYEDGE}
   };
   auto gpioEdgeOpt = utilities::TableConverter::convert(edgeTable, edge);
   if (!gpioEdgeOpt.has_value())
   {
      AIO_LOGE(TAG, "[%s] Invalid interrupt edge (%d)", m_name.c_str(), static_cast<int>(edge));
      return false;
   }

   if (!createTask())
   {
      AIO_LOGE(TAG, "[%s] Failed to create interrupt task", m_name.c_str());
      return false;
   }
   esp_err_t err = gpio_isr_handler_add(static_cast<gpio_num_t>(m_id),
                                        gpio_soc_isr_handler,
                                        this);
   if (err != ESP_OK)
   {
      AIO_LOGE(TAG, "[%s] Failed to add ISR handler", m_name.c_str());
      disableInterrupts();
      return false;
   }
   esp_err_t intrErr = gpio_set_intr_type(static_cast<gpio_num_t>(m_id), gpioEdgeOpt.value());
   if (intrErr != ESP_OK)
   {
      AIO_LOGE(TAG, "[%s] Failed to set interrupt type", m_name.c_str());
      disableInterrupts();
      return false;
   }
   return true;
}
bool socGPIO::disableInterrupts()
{
   bool result =  gpio_isr_handler_remove(static_cast<gpio_num_t>(m_id)) == ESP_OK &&
                  gpio_set_intr_type(static_cast<gpio_num_t>(m_id), GPIO_INTR_DISABLE) == ESP_OK;
   destroyTask();
   return result;
}
bool socGPIO::setPullMode(PullMode mode)
{
   static const std::vector<std::pair<PullMode, gpio_pull_mode_t>> pullModeTable = {
       {PullMode::NONE, GPIO_FLOATING},
       {PullMode::PULL_UP, GPIO_PULLUP_ONLY},
       {PullMode::PULL_DOWN, GPIO_PULLDOWN_ONLY}
   };
   auto gpioPullModeOpt = utilities::TableConverter::convert(pullModeTable, mode);
   if (!gpioPullModeOpt.has_value())
   {
      AIO_LOGE(TAG, "[%s] Invalid pull mode (%d)", m_name.c_str(), static_cast<int>(mode));
      return false;
   }
   return gpio_set_pull_mode(static_cast<gpio_num_t>(m_id), gpioPullModeOpt.value()) == ESP_OK;
}
bool socGPIO::setDirection(Direction direction)
{
   gpio_mode_t gpioMode = (direction == Direction::INPUT) ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT;
   return gpio_set_direction(static_cast<gpio_num_t>(m_id), gpioMode) == ESP_OK;
}

} // namespace drivers::gpio

