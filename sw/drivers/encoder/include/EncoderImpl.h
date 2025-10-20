#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "IEncoder.h"

namespace drivers::encoder
{

#define PIN_A_RISING  0x01
#define PIN_A_FALLING 0x02
#define PIN_B_RISING  0x03
#define PIN_B_FALLING 0x04

class EncoderImpl : public IEncoder
{
public:
   EncoderImpl(drivers::gpio::mcp23017::IMCP23017&, IEncoderListener&);
   ~EncoderImpl();
   EncoderImpl() = delete;
   EncoderImpl(EncoderImpl&&) = delete;
   EncoderImpl(const EncoderImpl&) = delete;
   EncoderImpl& operator=(EncoderImpl&&) = delete;
   EncoderImpl& operator=(const EncoderImpl&) = delete;

   void onInterrupt(drivers::gpio::mcp23017::PIN, bool state);
private:
   void openEncoderPin(drivers::gpio::mcp23017::PIN pin);
   void closeEncoderPin(drivers::gpio::mcp23017::PIN pin);
   void checkEncoderMove();

   drivers::gpio::mcp23017::IMCP23017& m_driver;
   IEncoderListener& m_listener;
   SemaphoreHandle_t m_edgesMutex;
   uint32_t m_lastEdges;
};

}
