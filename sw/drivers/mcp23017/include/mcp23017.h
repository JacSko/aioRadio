#pragma once

#include <optional>
#include <array>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "IMCP23017.h"

namespace drivers::gpio::mcp23017
{

class mcp23017 : public IMCP23017
{
public:

   mcp23017() = delete;
   mcp23017(mcp23017&&) = delete;
   mcp23017(const mcp23017&) = delete;
   mcp23017& operator=(mcp23017&&) = delete;
   mcp23017& operator=(const mcp23017&) = delete;

   mcp23017(int bus, const i2cConfig& config);
   mcp23017(i2c_master_bus_handle_t* handle, const i2cConfig& config);
   ~mcp23017();

   // ISR handler called on interrupt, do not call directly
   void onInterrupt();
   // Task function for handling interrupts, do not call directly
   void task();
   // Function processing the interrupt, do not call directly
   void processInterrupt();
private:
   bool probe() override;
   void reset() override;
   bool setDirection(PIN, Direction) override;
   bool setPolarity(PIN, Polarity) override;
   bool setPullup(PIN, Pullup) override;
   bool setInterruptCompare(PIN, InterruptCompare) override;
   bool writePin(PIN, bool) override;
   std::optional<bool> readPin(PIN) override;
   void setListener(PIN id, Listener listener, void* context) override;
   void removeListener(PIN id) override;

   struct StateChangeListener
   {
      Listener listener;
      void* context;
   };

   void onDriverCreation(const i2cConfig&);
   bool isDeviceConfigValid(const i2cConfig& config);
   bool isInterruptPinValid(uint16_t config);
   bool addBus(int bus, const i2cConfig& config);
   void removeBus();
   bool addDevice(const i2cConfig& config);
   void removeDevice();
   bool openResetPin();
   void closeResetPin();
   bool attachInterrupt();
   void detachInterrupt();
   void setDefaultConfiguration();
   bool writeRegister(Register, const uint8_t* data, size_t size);
   bool readRegister(Register, uint8_t*, size_t);
   void notifyListeners(PIN pin, bool state);

   bool readAndSetBitValue(Register reg, uint8_t bitMask, bool value);
   bool setBitValue(Register reg, uint8_t currentValue, uint8_t bitMask, bool value);

   i2c_master_bus_handle_t m_busHandle;
   i2cConfig m_i2cConfig;
   i2c_master_dev_handle_t m_deviceHandle;
   bool m_interruptAttached;
   bool m_resetPinOpened;
   TaskHandle_t m_taskHandle;
   SemaphoreHandle_t m_deviceMutex;
   SemaphoreHandle_t m_listenersMutex;
   SemaphoreHandle_t m_gpioStateMutex;
   uint16_t m_lastGpioState;
   std::array<StateChangeListener, static_cast<uint16_t>(PIN::GPIO_NUM)> m_listeners = {nullptr};
};

}
