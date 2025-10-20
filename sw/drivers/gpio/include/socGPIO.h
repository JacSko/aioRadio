#pragma once
#include <string>

#include "IGPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

namespace drivers::gpio
{

class socGPIO : public IGPIO
{
public:
   socGPIO() = delete;
   socGPIO(socGPIO&&) = delete;
   socGPIO(const socGPIO&) = delete;
   socGPIO& operator=(socGPIO&&) = delete;
   socGPIO& operator=(const socGPIO&) = delete;

   socGPIO(uint16_t gpio, const std::string&& name);
   ~socGPIO();
   void onInterrupt();
   void task();
   void handleInterrupt();
private:
   bool isConfigValid(const Config& config);
   bool configure(const Config& config) override;
   bool setState(State state) override;
   std::optional<State> getState() override;
   bool enableInterrupts(GPIOListener* listener, InterruptEdge edge) override;
   bool disableInterrupts() override;
   bool setPullMode(PullMode mode) override;
   bool setDirection(Direction direction) override;
   bool createTask();
   void destroyTask();

   const uint16_t m_id;
   const std::string m_name;
   GPIOListener* m_listener = nullptr;
   TaskHandle_t m_task;
   SemaphoreHandle_t m_listenersMutex;
};

}
