#pragma once
#include <cstdint>
#include <cstddef>

#include "TunerTypes.h"
#include "GPIOProvider.h"

namespace tuner::devicedriver
{

struct FrameTransmit
{
   const uint8_t* txBuffer;
   size_t txSize;
   uint8_t* rxBuffer;
   size_t rxSize;
   EventType eventType;
   uint32_t eventTimeout;
};


class EventHandler
{
public:
   virtual ~EventHandler() = default;
   virtual void onEvent(EventType type) = 0;
};

class IDeviceDriver
{
public:
   static std::unique_ptr<IDeviceDriver> create(drivers::gpio::GPIOProvider& gpioProvider);
   virtual ~IDeviceDriver() = default;
   virtual bool initialize() = 0;
   virtual void finalize() = 0;
   virtual bool setMode(TunerType type) = 0;
   virtual bool transmit(FrameTransmit&) = 0;
   virtual void setEventHandler(EventType type, EventHandler* handler) = 0;
   virtual void removeEventHandler(EventType type) = 0;
};

}
