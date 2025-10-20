#include <array>

#include "driver/gpio.h"

#include "mcp23017.h"
#include "TableConverter.hpp"
#include "Log.h"

namespace drivers::gpio::mcp23017
{

static const char* TAG = "mcp23017";
static const int XFER_TIMEOUT_MS = 20;
static const int TASK_STACK_SIZE = 4096;
static const int TASK_NOTIFY_WAIT_MS = 1000;
static const int TASK_PRIORITY = 10;

constexpr const char* toString(PIN pin)
{
   switch (pin)
   {
      case PIN::GPIOA0: return "GPIOA0";
      case PIN::GPIOA1: return "GPIOA1";
      case PIN::GPIOA2: return "GPIOA2";
      case PIN::GPIOA3: return "GPIOA3";
      case PIN::GPIOA4: return "GPIOA4";
      case PIN::GPIOA5: return "GPIOA5";
      case PIN::GPIOA6: return "GPIOA6";
      case PIN::GPIOA7: return "GPIOA7";
      case PIN::GPIOB0: return "GPIOB0";
      case PIN::GPIOB1: return "GPIOB1";
      case PIN::GPIOB2: return "GPIOB2";
      case PIN::GPIOB3: return "GPIOB3";
      case PIN::GPIOB4: return "GPIOB4";
      case PIN::GPIOB5: return "GPIOB5";
      case PIN::GPIOB6: return "GPIOB6";
      case PIN::GPIOB7: return "GPIOB7";
      default: return "Unknown";
   }
}
constexpr const char* toString(Register reg)
{
   switch (reg)
   {
      case Register::IODIRA: return "IODIRA";
      case Register::IODIRB: return "IODIRB";
      case Register::IPOLA: return "IPOLA";
      case Register::IPOLB: return "IPOLB";
      case Register::GPINTENA: return "GPINTENA";
      case Register::GPINTENB: return "GPINTENB";
      case Register::DEFVALA: return "DEFVALA";
      case Register::DEFVALB: return "DEFVALB";
      case Register::INTCONA: return "INTCONA";
      case Register::INTCONB: return "INTCONB";
      case Register::IOCON: return "IOCON";
      case Register::GPPUA: return "GPPUA";
      case Register::GPPUB: return "GPPUB";
      case Register::INTFA: return "INTFA";
      case Register::INTFB: return "INTFB";
      case Register::INTCAPA: return "INTCAPA";
      case Register::INTCAPB: return "INTCAPB";
      case Register::GPIOA: return "GPIOA";
      case Register::GPIOB: return "GPIOB";
      case Register::OLATA: return "OLATA";
      case Register::OLATB: return "OLATB";
      default: return "Unknown";
   }
}
constexpr const char* toString(Direction dir)
{
   switch (dir)
   {
      case Direction::INPUT: return "INPUT";
      case Direction::OUTPUT: return "OUTPUT";
      default: return "Unknown";
   }
}
constexpr const char* toString(Polarity pol)
{
   switch (pol)
   {
      case Polarity::NORMAL: return "NORMAL";
      case Polarity::INVERTED: return "INVERTED";
      default: return "Unknown";
   }
}
constexpr const char* toString(InterruptOnChange ioc)
{
   switch (ioc)
   {
      case InterruptOnChange::DISABLED: return "DISABLED";
      case InterruptOnChange::ENABLED: return "ENABLED";
      default: return "Unknown";
   }
}
constexpr const char* toString(InterruptCompare ic)
{
   switch (ic)
   {
      case InterruptCompare::COMPARE_DEFAULT: return "COMPARE_DEFAULT";
      case InterruptCompare::COMPARE_PREVIOUS: return "COMPARE_PREVIOUS";
      default: return "Unknown";
   }
}
constexpr const char* toString(Pullup pu)
{
   switch (pu)
   {
      case Pullup::DISABLED: return "DISABLED";
      case Pullup::ENABLED: return "ENABLED";
      default: return "Unknown";
   }
}
constexpr const char* toString(InterruptFlag flag)
{
   switch (flag)
   {
      case InterruptFlag::NO_INTERRUPT: return "NO_INTERRUPT";
      case InterruptFlag::INTERRUPT_OCCURRED: return "INTERRUPT_OCCURRED";
      default: return "Unknown";
   }
}
constexpr const char* toString(InterruptEdge edge)
{
   switch (edge)
   {
      case InterruptEdge::RISING: return "RISING";
      case InterruptEdge::FALLING: return "FALLING";
      case InterruptEdge::BOTH: return "BOTH";
      default: return "Unknown";
   }
}
constexpr const char* toString(PullMode mode)
{
   switch (mode)
   {
      case PullMode::NONE: return "NONE";
      case PullMode::PULL_UP: return "PULL_UP";
      case PullMode::PULL_DOWN: return "PULL_DOWN";
      default: return "Unknown";
   }
}

static void IRAM_ATTR gpio_i2c_isr_handler(void* arg)
{
   //This is called in ISR contextESP_DRAM_LOGx
   mcp23017* i2cDriver = static_cast<mcp23017*>(arg);
   i2cDriver->onInterrupt();
}

static void taskFunction(void* arg)
{
   mcp23017* i2cDriver = static_cast<mcp23017*>(arg);
   i2cDriver->task();
}

std::unique_ptr<IMCP23017> IMCP23017::create(int bus, const i2cConfig& config)
{
   return std::make_unique<mcp23017>(bus, config);
}
std::unique_ptr<IMCP23017> IMCP23017::create(i2c_master_bus_handle_t* handle, const i2cConfig& config)
{
   return std::make_unique<mcp23017>(handle, config);
}

mcp23017::mcp23017(int bus, const i2cConfig& config):
m_busHandle(nullptr),
m_deviceHandle(nullptr),
m_interruptAttached(false),
m_resetPinOpened(false),
m_taskHandle(nullptr),
m_deviceMutex(xSemaphoreCreateMutex()),
m_listenersMutex(xSemaphoreCreateMutex()),
m_gpioStateMutex(xSemaphoreCreateMutex()),
m_lastGpioState(0)
{
   assert(m_deviceMutex != nullptr);
   assert(m_listenersMutex != nullptr);
   assert(m_gpioStateMutex != nullptr);

   AIO_LOGI(TAG, "mcp23017 initialization with bus creation, interrupts? %s",
         config.interruptPin ? "YES" : "NO");
   if (!isDeviceConfigValid(config))
   {
      AIO_LOGE(TAG, "Invalid I2C config!");
      return;
   }
   m_i2cConfig = config;

   if (!addBus(bus, config))
   {
      AIO_LOGE(TAG, "Failed to create I2C master bus");
      return;
   }
   onDriverCreation(config);
}

mcp23017::mcp23017(i2c_master_bus_handle_t* handle, const i2cConfig& config):
m_deviceHandle(nullptr),
m_interruptAttached(false),
m_resetPinOpened(false),
m_taskHandle(nullptr),
m_deviceMutex(xSemaphoreCreateMutex()),
m_listenersMutex(xSemaphoreCreateMutex()),
m_gpioStateMutex(xSemaphoreCreateMutex()),
m_lastGpioState(0)
{
   assert(m_deviceMutex != nullptr);
   assert(m_listenersMutex != nullptr);
   assert(m_gpioStateMutex != nullptr);

   AIO_LOGI(TAG, "mcp23017 initialization with existing bus, interrupts? %s",
         config.interruptPin ? "YES" : "NO");
   if (!handle)
   {
      AIO_LOGE(TAG, "Handle cannot be null!");
      return;
   }
   m_busHandle = *handle;
   if (!isDeviceConfigValid(config))
   {
      AIO_LOGE(TAG, "Invalid I2C config!");
      return;
   }
   m_i2cConfig = config;
   onDriverCreation(config);
}
mcp23017::~mcp23017()
{
   if (m_interruptAttached)
   {
      detachInterrupt();
   }
   if (m_taskHandle)
   {
      vTaskDelete(m_taskHandle);
      m_taskHandle = nullptr;
   }
   removeDevice();
   removeBus();
   if (m_resetPinOpened)
   {
      closeResetPin();
      m_resetPinOpened = false;
   }
}
void mcp23017::onDriverCreation(const i2cConfig& config)
{
   if (!openResetPin())
   {
      AIO_LOGE(TAG, "Failed to open reset pin");
      return;
   }
   if (!addDevice(config))
   {
      AIO_LOGE(TAG, "Failed to add I2C device");
      return;
   }
   reset();
   if (config.interruptPin && isInterruptPinValid(*config.interruptPin))
   {
      BaseType_t result = xTaskCreate(taskFunction, "MCP23017", TASK_STACK_SIZE, this, TASK_PRIORITY, &m_taskHandle);
      AIO_LOGE_IF(result != pdPASS, TAG, "Failed to create interrupt task, error %d", result);
      if (result == pdPASS && (!attachInterrupt()))
      {
         AIO_LOGE(TAG, "Failed to attach interrupt, deleting task!");
         vTaskDelete(m_taskHandle);
         m_taskHandle = nullptr;
      }
   }
   setDefaultConfiguration();
   AIO_LOGI(TAG, "mcp23017 initialized");
}
bool mcp23017::addBus(int bus, const i2cConfig& config)
{
   AIO_LOGD(TAG, "Creating I2C master bus %d on SCL GPIO%d, SDA GPIO%d",
         bus, config.sclPin, config.sdaPin);
   i2c_master_bus_config_t busConfig = {
      .i2c_port = static_cast<i2c_port_num_t>(bus),
      .sda_io_num = static_cast<gpio_num_t>(config.sdaPin),
      .scl_io_num = static_cast<gpio_num_t>(config.sclPin),
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 1,
   };

   esp_err_t result = i2c_new_master_bus(&busConfig, &m_busHandle);
   if (result != ESP_OK)
   {
      AIO_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(result));
      m_busHandle = {};
      return false;
   }
   AIO_LOGI(TAG, "I2C master bus created on SCL GPIO%d, SDA GPIO%d",
         config.sclPin, config.sdaPin);
   return true;
}
bool mcp23017::addDevice(const i2cConfig& config)
{
   AIO_LOGD(TAG, "Adding I2C device 0x%02X to bus", config.i2cAddress);
   i2c_device_config_t deviceConfig = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = config.i2cAddress,
      .scl_speed_hz = config.clockSpeedHz,
      .scl_wait_us = 0,
      .flags = { .disable_ack_check = 0 },
   };
   esp_err_t result = i2c_master_bus_add_device(m_busHandle, &deviceConfig, &m_deviceHandle);
   if (result != ESP_OK)
   {
      AIO_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(result));
      m_deviceHandle = {};
      return false;
   }
   AIO_LOGI(TAG, "I2C[0x%02X] device added", config.i2cAddress);
   return true;
}
void mcp23017::removeDevice()
{
   AIO_LOGD(TAG, "Removing I2C device 0x%02X from bus", m_i2cConfig.i2cAddress);
   if (m_deviceHandle != nullptr)
   {
      i2c_master_bus_rm_device(m_deviceHandle);
      m_deviceHandle = nullptr;
   }
}
void mcp23017::removeBus()
{
   AIO_LOGD(TAG, "Removing I2C master bus");
   if (m_busHandle != nullptr)
   {
      i2c_del_master_bus(m_busHandle);
      m_busHandle = {};
   }
}
bool mcp23017::isDeviceConfigValid(const i2cConfig& config)
{
   if (!GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(config.sclPin)))
   {
      AIO_LOGE(TAG, "Invalid GPIO for SCL (%d)",
            static_cast<uint32_t>(config.sclPin));
      return false;
   }

   if (!GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(config.sdaPin)))
   {
      AIO_LOGE(TAG, "Invalid GPIO for SDA (%d)",
            static_cast<uint32_t>(config.sdaPin));
      return false;
   }

   if (!GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(config.resetPin)))
   {
      AIO_LOGE(TAG, "Invalid GPIO for RESET (%d)",
            static_cast<uint32_t>(config.resetPin));
      return false;
   }
   if (config.clockSpeedHz == 0)
   {
      AIO_LOGE(TAG, "Invalid SCL speed (%uHz)", config.clockSpeedHz);
      return false;
   }
   return true;
}
bool mcp23017::isInterruptPinValid(uint16_t pin)
{
   if (!GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(pin)))
   {
      AIO_LOGE(TAG, "Invalid GPIO for interrupt (%d)", pin);
      return false;
   }
   return true;
}
bool mcp23017::attachInterrupt()
{
   AIO_LOGD(TAG, "Attaching interrupt on GPIO%d", *m_i2cConfig.interruptPin);
   gpio_config_t io_conf = {};
   io_conf.pin_bit_mask = (1ULL << *m_i2cConfig.interruptPin);
   io_conf.mode = GPIO_MODE_INPUT;
   io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
   io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
   io_conf.intr_type = GPIO_INTR_NEGEDGE;

   esp_err_t err = gpio_config(&io_conf);
   if (err != ESP_OK)
   {
      AIO_LOGE(TAG, "Failed to configure GPIO%d", *m_i2cConfig.interruptPin);
      return false;
   }

   err = gpio_isr_handler_add(static_cast<gpio_num_t>(*m_i2cConfig.interruptPin),
         gpio_i2c_isr_handler,
         this);
   if (err != ESP_OK)
   {
      AIO_LOGE(TAG, "Failed to add ISR handler for GPIO%d", *m_i2cConfig.interruptPin);
      return false;
   }
   m_interruptAttached = true;
   return true;
}
bool mcp23017::openResetPin()
{
   gpio_config_t io_conf = {};
   io_conf.pin_bit_mask = (1ULL << m_i2cConfig.resetPin);
   io_conf.mode = GPIO_MODE_OUTPUT;
   io_conf.intr_type = GPIO_INTR_DISABLE;
   esp_err_t err = gpio_config(&io_conf);
   if (err != ESP_OK)
   {
      AIO_LOGE(TAG, "Failed to configure GPIO%d", m_i2cConfig.resetPin);
      return false;
   }
   AIO_LOGI(TAG, "I2C[0x%02X] reset pin created", m_i2cConfig.i2cAddress);
   m_resetPinOpened = true;
   return true;
}
void mcp23017::closeResetPin()
{
   if (GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(m_i2cConfig.resetPin)))
   {
      gpio_reset_pin(static_cast<gpio_num_t>(m_i2cConfig.resetPin));
   }
}
void mcp23017::reset()
{
   static const TickType_t resetDelay = 500 / portTICK_PERIOD_MS;
   AIO_LOGI(TAG, "I2C[0x%02X] resetting device", m_i2cConfig.i2cAddress);
   gpio_set_level(static_cast<gpio_num_t>(m_i2cConfig.resetPin), false);
   vTaskDelay(resetDelay);
   gpio_set_level(static_cast<gpio_num_t>(m_i2cConfig.resetPin), true);
   AIO_LOGI(TAG, "I2C[0x%02X] reset done", m_i2cConfig.i2cAddress);
}
void mcp23017::onInterrupt()
{
   BaseType_t taskWoken = pdFALSE;
   vTaskNotifyGiveFromISR(m_taskHandle, &taskWoken);
   portYIELD_FROM_ISR(taskWoken);
}
void mcp23017::processInterrupt()
{
   AIO_LOGV(TAG, "Processing MCP23017 interrupt");
   xSemaphoreTake(m_gpioStateMutex, portMAX_DELAY);
   uint8_t gpioStates[2] = {};
   if (readRegister(Register::GPIOA, gpioStates, sizeof(gpioStates)))
   {
      uint16_t currentGpioState = (static_cast<uint16_t>(gpioStates[1]) << 8) | gpioStates[0];
      uint16_t changedPins = currentGpioState ^ m_lastGpioState;
      // AIO_LOGV(TAG, "Current GPIO state: 0x%04X, changed: 0x%04X, calling listeners",
      //       currentGpioState, changedPins);
      for (uint16_t pin = 0; pin < static_cast<uint16_t>(PIN::GPIO_NUM); ++pin)
      {
         if (changedPins & (1 << pin))
         {
            bool pinState = (currentGpioState & (1 << pin)) != 0;
            notifyListeners(static_cast<PIN>(pin), pinState);
         }
      }
      m_lastGpioState = currentGpioState;
   }
   else
   {
      AIO_LOGE(TAG, "Failed to read GPIO states on interrupt");
   }
   xSemaphoreGive(m_gpioStateMutex);
}
void mcp23017::task()
{
   AIO_LOGI(TAG, "starting MCP23017 interrupt task");
   for(;;)
   {
      int eventReady = ulTaskNotifyTake(true, pdMS_TO_TICKS(TASK_NOTIFY_WAIT_MS));
      AIO_LOGV_IF(eventReady, TAG, "got %d events from interrupt", eventReady);
      AIO_LOGD_IF(!eventReady, TAG, "no interrupt in last %dms", TASK_NOTIFY_WAIT_MS);
      if (eventReady)
      {
         processInterrupt();
      }
   }
}
void mcp23017::detachInterrupt()
{
   AIO_LOGD(TAG, "Detaching interrupt on GPIO%d", *m_i2cConfig.interruptPin);
   m_interruptAttached = false;
   gpio_isr_handler_remove(static_cast<gpio_num_t>(*m_i2cConfig.interruptPin));
   gpio_reset_pin(static_cast<gpio_num_t>(*m_i2cConfig.interruptPin));
}
void mcp23017::setDefaultConfiguration()
{
   // Default configuration:
   // INTPOL = 0;
   // ODR = 0;
   // HAEN = 0;
   // DISSLW = 0;
   // SEQOP = 0;
   // MIRROR = 1;
   // BANK = 0;
   AIO_LOGD(TAG, "Setting default MCP23017 configuration");
   uint8_t configValue = 0b01000000;
   writeRegister(Register::IOCON, &configValue, sizeof(configValue));

   std::array<uint8_t, 2> allPullupsEnabled = {0xFF, 0xFF};
   writeRegister(Register::GPPUA, allPullupsEnabled.data(), allPullupsEnabled.size());
   if (m_interruptAttached)
   {
      std::array<uint8_t, 2> allInterruptsEnabled = {0xFF, 0xFF};
      writeRegister(Register::GPINTENA, allInterruptsEnabled.data(), allInterruptsEnabled.size());
   }
}
bool mcp23017::probe()
{
   return i2c_master_probe(m_busHandle, m_i2cConfig.i2cAddress, XFER_TIMEOUT_MS) == ESP_OK;
}
bool mcp23017::setBitValue(Register reg, uint8_t currentValue, uint8_t bitMask, bool value)
{
   currentValue &= ~bitMask;
   if (value)
   {
      currentValue |= bitMask;
   }
   return writeRegister(reg, &currentValue, sizeof(currentValue));
}
bool mcp23017::readAndSetBitValue(Register reg, uint8_t bitMask, bool value)
{
   xSemaphoreTake(m_deviceMutex, portMAX_DELAY);
   uint8_t currentValue = 0;
   if (!readRegister(reg, &currentValue, sizeof(currentValue)))
   {
      AIO_LOGE(TAG, "Failed to read register 0x%02X", static_cast<uint8_t>(reg));
      return false;
   }
   bool result = setBitValue(reg, currentValue, bitMask, value);
   xSemaphoreGive(m_deviceMutex);
   return result;
}
bool mcp23017::setDirection(PIN pin, Direction direction)
{
   AIO_LOGD(TAG, "Setting direction of pin %s to %s", toString(pin),
         toString(direction));
   Register reg = (pin < PIN::GPIOB0) ? Register::IODIRA : Register::IODIRB;
   uint8_t pinMask = 1 << (static_cast<uint16_t>(pin) % 8);
   return readAndSetBitValue(reg, pinMask, direction == Direction::INPUT);
}
bool mcp23017::setPolarity(PIN pin, Polarity polarity)
{
   AIO_LOGD(TAG, "Setting polarity of pin %s to %s", toString(pin),
         toString(polarity));
   Register reg = (pin < PIN::GPIOB0) ? Register::IPOLA : Register::IPOLB;
   uint8_t pinMask = 1 << (static_cast<uint16_t>(pin) % 8);
   return readAndSetBitValue(reg, pinMask, polarity == Polarity::INVERTED);
}
bool mcp23017::setPullup(PIN pin, Pullup pullup)
{
   AIO_LOGD(TAG, "Setting pull-up of pin %s to %s", toString(pin),
         toString(pullup));
   Register reg = (pin < PIN::GPIOB0) ? Register::GPPUA : Register::GPPUB;
   uint8_t pinMask = 1 << (static_cast<uint16_t>(pin) % 8);
   return readAndSetBitValue(reg, pinMask, pullup == Pullup::ENABLED);
}
bool mcp23017::setInterruptCompare(PIN pin, InterruptCompare compare)
{
   AIO_LOGD(TAG, "Setting interrupt compare of pin %s to %s", toString(pin),
         toString(compare));
   Register reg = (pin < PIN::GPIOB0) ? Register::INTCONA : Register::INTCONB;
   uint8_t pinMask = 1 << (static_cast<uint16_t>(pin) % 8);
   return readAndSetBitValue(reg, pinMask, compare == InterruptCompare::COMPARE_DEFAULT);
}
bool mcp23017::writePin(PIN pin, bool state)
{
   AIO_LOGD(TAG, "Writing pin %s to state %s", toString(pin),
         state ? "HIGH" : "LOW");
   xSemaphoreTake(m_gpioStateMutex, portMAX_DELAY);
   Register registerToChange = (pin < PIN::GPIOB0) ? Register::GPIOA : Register::GPIOB;
   uint8_t pinMask = 1 << (static_cast<uint16_t>(pin) % 8);
   uint8_t currentRegisterValue = 0;
   if (m_interruptAttached)
   {
      currentRegisterValue = (m_lastGpioState >> ((pin < PIN::GPIOB0) ? 0 : 8)) & 0xFF;
   }
   else
   {
      if (!readRegister(registerToChange, &currentRegisterValue, sizeof(currentRegisterValue)))
      {
         AIO_LOGE(TAG, "Failed to read GPIO register before writing %s", toString(pin));
         xSemaphoreGive(m_gpioStateMutex);
         return false;
      }
   }

   if (!setBitValue(registerToChange, currentRegisterValue, pinMask, state))
   {
      AIO_LOGE(TAG, "Failed to write register %s for pin %s", toString(registerToChange),
            toString(pin));
      xSemaphoreGive(m_gpioStateMutex);
      return false;
   }
   m_lastGpioState &= ~(1 << static_cast<uint16_t>(pin));
   if (state)
   {
      m_lastGpioState |= (1 << static_cast<uint16_t>(pin));
   }
   xSemaphoreGive(m_gpioStateMutex);
   return true;
}
std::optional<bool> mcp23017::readPin(PIN pin)
{
   AIO_LOGD(TAG, "Reading pin %s state", toString(pin));
   xSemaphoreTake(m_gpioStateMutex, portMAX_DELAY);
   if (m_interruptAttached)
   {
      // No need to read from device, use cached state
      return m_lastGpioState & (1 << static_cast<uint16_t>(pin));
   }

   Register registerToRead = (pin < PIN::GPIOB0) ? Register::GPIOA : Register::GPIOB;
   uint8_t registerValue = 0;
   if (!readRegister(Register::GPIOA, &registerValue, sizeof(registerValue)))
   {
      AIO_LOGE(TAG, "Failed to read register %s for pin %s", toString(registerToRead),
            toString(pin));
      return std::nullopt;
   }
   xSemaphoreGive(m_gpioStateMutex);
   return registerValue & (1 << static_cast<uint16_t>(pin));
}
bool mcp23017::writeRegister(Register reg, const uint8_t* data, size_t size)
{
   static constexpr int MAX_DATA_SIZE = 3;
   if (size == 0 || size > 2)
   {
      AIO_LOGE(TAG, "Invalid data size %d for %s", static_cast<int>(size),
            toString(reg));
      return false;
   }
   if (!data)
   {
      AIO_LOGE(TAG, "Data pointer is null for register %s", toString(reg));
      return false;
   }

   AIO_LOGV(TAG, "Writing %d bytes to register %s [%x %x]", static_cast<int>(size),
         toString(reg), data[0], size > 1 ? data[1] : 0);
   uint8_t buffer[MAX_DATA_SIZE] = {};
   buffer[0] = static_cast<uint8_t>(reg);
   for (size_t i = 0; i < size; ++i)
   {
      buffer[i + 1] = data[i];
   }
   esp_err_t result = i2c_master_transmit(m_deviceHandle, buffer, 1 + size, XFER_TIMEOUT_MS);
   return result == ESP_OK;
}
bool mcp23017::readRegister(Register reg, uint8_t* data, size_t size)
{
   if (size == 0 || size > 2)
   {
      AIO_LOGE(TAG, "Invalid data size %d for %s", static_cast<int>(size),
            toString(reg));
      return false;
   }
   if (!data)
   {
      AIO_LOGE(TAG, "Data pointer is null for register %s", toString(reg));
      return false;
   }

   AIO_LOGV(TAG, "Reading %d bytes from register %s", static_cast<int>(size),
         toString(reg));
   uint8_t regToRead = static_cast<uint8_t>(reg);
   esp_err_t result = i2c_master_transmit_receive(m_deviceHandle, &regToRead, 1, data, size, XFER_TIMEOUT_MS);
   return result == ESP_OK;
}
void mcp23017::notifyListeners(PIN id, bool value)
{
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   if (m_listeners[static_cast<uint16_t>(id)].listener != nullptr)
   {
      m_listeners[static_cast<uint16_t>(id)].listener(id, value, m_listeners[static_cast<uint16_t>(id)].context);
   }
   xSemaphoreGive(m_listenersMutex);
}
void mcp23017::setListener(PIN id, Listener listener, void* context)
{
   if (!listener)
   {
      AIO_LOGE(TAG, "Listener is null, cannot set listener for pin %s", toString(id));
      return;
   }
   AIO_LOGD(TAG, "Setting listener for pin %s", toString(id));
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   if (m_listeners[static_cast<uint16_t>(id)].listener != nullptr)
   {
      AIO_LOGW(TAG, "Overwriting existing listener for %s", toString(id));
   }
   m_listeners[static_cast<uint16_t>(id)].listener = listener;
   m_listeners[static_cast<uint16_t>(id)].context = context;
   xSemaphoreGive(m_listenersMutex);
}
void mcp23017::removeListener(PIN id)
{
   AIO_LOGD(TAG, "REmoving listener for pin %s", toString(id));
   xSemaphoreTake(m_listenersMutex, portMAX_DELAY);
   m_listeners[static_cast<uint16_t>(id)] = {};
   xSemaphoreGive(m_listenersMutex);
}

} // namespace drivers::gpio::mcp23017
