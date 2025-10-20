#pragma once
/**
 * @file IGPIO.h
 * @brief Interface for GPIO controlling.
 *
 * This class allows to control the GPIO pins providing abstraction over the ESP32 and MCP23017 GPIOs.
 * Supports on-change notifications over registered listener. Listener is called from the dedicated GPIO
 * task, which is created when the interrupt is enabled. It means, that it is relatively safe to perform some operations
 * in the listener thread, but it is still recommended to keep time-consuming operations out of the listener context since
 * it may block the GPIO processing queue.
 *
 * @author Jacek Skowronek (jacekskowronekk@gmail.com)
 */
#include <optional>

#include "GPIOMapping.h"

namespace drivers::gpio
{

enum class Direction
{
   INPUT,
   OUTPUT
};

enum class State
{
   LOW,
   HIGH
};

enum class InterruptEdge
{
   RISING,
   FALLING,
   BOTH
};

enum class PullMode
{
   NONE,
   PULL_UP,
   PULL_DOWN
};

class GPIOListener
{
public:
   virtual ~GPIOListener() = default;
   virtual void onInterrupt(GPIO id, State state) = 0;
};

struct Config
{
   Direction direction = Direction::INPUT;
   PullMode pullMode = PullMode::NONE;
   State state = State::LOW;
   InterruptEdge interruptEdge = InterruptEdge::BOTH;
   GPIOListener* listener = nullptr;
};

class IGPIO
{
public:
   virtual ~IGPIO() = default;
   virtual bool configure(const Config& config) = 0;
   virtual bool setState(State state) = 0;
   virtual std::optional<State> getState() = 0;
   virtual bool enableInterrupts(GPIOListener* listener, InterruptEdge edge) = 0;
   virtual bool disableInterrupts() = 0;
   virtual bool setPullMode(PullMode mode) = 0;
   virtual bool setDirection(Direction direction) = 0;
};

} // namespace drivers::gpio
