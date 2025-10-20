#pragma once
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "IGPIO.h"
#include "IMCP23017.h"

namespace drivers::gpio
{

class i2cGPIO : public IGPIO
{
public:
   i2cGPIO() = delete;
   i2cGPIO(i2cGPIO&&) = delete;
   i2cGPIO(const i2cGPIO&) = delete;
   i2cGPIO& operator=(i2cGPIO&&) = delete;
   i2cGPIO& operator=(const i2cGPIO&) = delete;

   i2cGPIO(uint16_t gpio, mcp23017::IMCP23017& driver, const std::string&& name);
   ~i2cGPIO();
   void onInterrupt(mcp23017::PIN pin, bool state);
   void handleInterrupt();
   void task();
private:
   bool isConfigValid(const Config& config);
   bool configure(const Config& config) override;
   bool setState(State state) override;
   std::optional<State> getState() override;
   bool enableInterrupts(GPIOListener* listener, InterruptEdge edge) override;
   bool disableInterrupts() override;
   bool setPullMode(PullMode mode) override;
   bool setDirection(Direction direction) override;
   bool createQueue();
   bool createTask();
   void destroyQueue();
   void destroyTask();
   bool isCorrectEdge(bool state);

   mcp23017::PIN m_id;
   mcp23017::IMCP23017& m_driver;
   const std::string m_name;
   Config m_config;
   InterruptEdge m_interruptEdge;
   bool m_lastInputState;
   GPIOListener* m_listener;
   TaskHandle_t m_task;
   QueueHandle_t m_queue;
   SemaphoreHandle_t m_listenersMutex;
};

}
